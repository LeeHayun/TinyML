import torch
import torch.nn as nn
import torch.nn.functional as F
from einops import rearrange


class NonOverlappedConv2d(nn.Module):
    def __init__(self, conv, args):
        super(NonOverlappedConv2d, self).__init__()
        self.conv = conv
        self.conv.padding = (0,0)
        self.ph = args.n_patch_h
        self.pw = args.n_patch_w
        self.coefficient_train = args.coefficient_train
        self.sampling = args.sampling
        self.sample_width = args.sample_width
        self.use_b_l = conv.stride[0] == 1
        self.pad_method = args.pad_method

        if self.pad_method == "predict":
            self.d = conv.weight.device
            v = args.coefficient_value
            pre_shape = (1, self.conv.in_channels, 1, 1)
            if args.coefficient_random_init:
                self.t = nn.Parameter(torch.randn(*pre_shape, 2, 1).to(self.d))
                self.b = nn.Parameter(torch.randn(*pre_shape, 2, 1).to(self.d))
                self.l = nn.Parameter(torch.randn(*pre_shape, 1, 2).to(self.d))
                self.r = nn.Parameter(torch.randn(*pre_shape, 1, 2).to(self.d))
            else:
                self.t = nn.Parameter(v * torch.ones(*pre_shape, 2, 1).to(self.d))
                self.b = nn.Parameter(v * torch.ones(*pre_shape, 2, 1).to(self.d))
                self.l = nn.Parameter(v * torch.ones(*pre_shape, 1, 2).to(self.d))
                self.r = nn.Parameter(v * torch.ones(*pre_shape, 1, 2).to(self.d))


    def _forward_non_overlapping(self, x):
        patch = rearrange(x, "B C (ph H) (pw W) -> B C ph pw H W", ph=self.ph, pw=self.pw)
        B, C, _, _, H, W = patch.size()

        if self.sampling:
            padded_x = F.pad(x, (1, 1, 1, 1), mode='constant', value=0.0)
            overlapped_patch = padded_x.unfold(2, H + 2, H).unfold(3, W + 2, W)
            # Pad Top, Left, Top-Left
            pad_t   = overlapped_patch[:, :, :, :,  0:1, 1:-1].clone()
            pad_l   = overlapped_patch[:, :, :, :, 1:-1,  0:1].clone()
            pad_t_l = overlapped_patch[:, :, :, : , 0:1,  0:1].clone()
            if self.sample_width > 1:
                if self.pad_method == "zero":
                    new_pad_t = 0
                    new_pad_l = 0
                elif self.pad_method == "mean":
                    new_pad_t = torch.mean(self.t * patch[:, :, :, :, :2,  1::2], dim=-2, keepdim=True)
                    new_pad_l = torch.mean(self.l * patch[:, :, :, :, 1::2,  :2], dim=-1, keepdim=True)
                elif self.pad_method == "replicate":
                    new_pad_t = patch[:, :, :, :, :1,  1::2].clone()
                    new_pad_l = patch[:, :, :, :, 1::2,  :1].clone()
                elif self.pad_method == "reflect":
                    new_pad_t = patch[:, :, :, :, 1:2,  1::2].clone()
                    new_pad_l = patch[:, :, :, :, 1::2,  1:2].clone()
                elif self.pad_method == "predict":
                    new_pad_t = torch.sum(self.t * patch[:, :, :, :, :2,  1::2], dim=-2, keepdim=True)
                    new_pad_l = torch.sum(self.l * patch[:, :, :, :, 1::2,  :2], dim=-1, keepdim=True)
                pad_t[:, :, :, :, :, 1::2] = new_pad_t
                pad_l[:, :, :, :, 1::2, :] = new_pad_l
        else:
            # If you use zero padding with non-sampling
            if self.pad_method == "zero":
                non_overlapped_patch = F.pad(patch, (1, 1, 1, 1), mode='constant', value=0.0)
                return non_overlapped_patch

            if self.pad_method == "mean":
                pad_t = torch.mean(patch[:, :, :, :,  :2,   :], dim=-2, keepdim=True)
                pad_l = torch.mean(patch[:, :, :, :,   :,  :2], dim=-1, keepdim=True)
                pad_t_l = patch[:, :, :, :,  :1,  :1].clone()
            elif self.pad_method == "replicate":
                pad_t = patch[:, :, :, :,  0:1,   :].clone()
                pad_l = patch[:, :, :, :,   :,  0:1].clone()
                pad_t_l = patch[:, :, :, :,  0:1,  0:1].clone()
            elif self.pad_method == "reflect":
                pad_t = patch[:, :, :, :,  1:2,   :].clone()
                pad_l = patch[:, :, :, :,   :,  1:2].clone()
                pad_t_l = patch[:, :, :, :,  1:2,  1:2].clone()
            elif self.pad_method == "predict":
                pad_t = torch.sum(self.t * patch[:, :, :, :,  :2,   :], dim=-2, keepdim=True)
                pad_l = torch.sum(self.l * patch[:, :, :, :,   :,  :2], dim=-1, keepdim=True)
                pad_t_l = patch[:, :, :, :,  :1,  :1].clone()

            # Boundary must be zero
            pad_t[:, :, 0, :, 0, :] *= 0.0
            pad_l[:, :, :, 0, :, 0] *= 0.0
            pad_t_l[:, :, 0, :, :, :] *= 0.0
            pad_t_l[:, :, :, 0, :, :] *= 0.0

        # When Stride=1
        if self.use_b_l:
            if self.pad_method == "zero":
                pad_b = torch.zeros(B, C, self.ph, self.pw, 1, W).to(self.d)
                pad_r = torch.zeros(B, C, self.ph, self.pw, H, 1).to(self.d)
                pad_corner = torch.zeros(B, C, self.ph, self.pw, 1, 1).to(self.d)
                pad_t_r = pad_corner
                pad_b_l = pad_corner
                pad_b_r = pad_corner
            elif self.pad_method == "mean":
                pad_b = torch.mean(patch[:, :, :, :, -2:,   :], dim=-2, keepdim=True)
                pad_r = torch.mean(patch[:, :, :, :,   :, -2:], dim=-1, keepdim=True)
                pad_t_r = patch[:, :, :, :,  0:1, -1:].clone()
                pad_b_l = patch[:, :, :, :, -1:,  0:1].clone()
                pad_b_r = patch[:, :, :, :, -1:, -1:].clone()
            elif self.pad_method == "replicate":
                pad_b = patch[:, :, :, :, -1:,   :].clone()
                pad_r = patch[:, :, :, :,   :, -1:].clone()
                pad_t_r = patch[:, :, :, :,  0:1, -1:].clone()
                pad_b_l = patch[:, :, :, :, -1:,  0:1].clone()
                pad_b_r = patch[:, :, :, :, -1:, -1:].clone()
            elif self.pad_method == "reflect":
                pad_b = patch[:, :, :, :, -2:-1,   :].clone()
                pad_r = patch[:, :, :, :,   :, -2:-1].clone()
                pad_t_r = patch[:, :, :, :,  1:2, -2:-1].clone()
                pad_b_l = patch[:, :, :, :, -2:-1,  1:2].clone()
                pad_b_r = patch[:, :, :, :, -2:-1, -2:-1].clone()
            elif self.pad_method == "predict":
                pad_b = torch.sum(self.b * patch[:, :, :, :, -2:,   :], dim=-2, keepdim=True)
                pad_r = torch.sum(self.r * patch[:, :, :, :,   :, -2:], dim=-1, keepdim=True)
                pad_t_r = patch[:, :, :, :,  0:1, -1:].clone()
                pad_b_l = patch[:, :, :, :, -1:,  0:1].clone()
                pad_b_r = patch[:, :, :, :, -1:, -1:].clone()

            # Boundary must be zero
            pad_b[:, :, -1,  :, -1,  :] *= 0.0
            pad_r[:, :,  :, -1,  :, -1] *= 0.0
            pad_t_r[:, :, 0,  :, :, :] *= 0.0
            pad_t_r[:, :, :, -1, :, :] *= 0.0
            pad_b_l[:, :, -1, :, :, :] *= 0.0
            pad_b_l[:, :,  :, 0, :, :] *= 0.0
            pad_b_r[:, :, -1,  :, :, :] *= 0.0
            pad_b_r[:, :,  :, -1, :, :] *= 0.0

            patch_top = torch.cat([pad_t_l, pad_t, pad_t_r], dim=-1)
            patch_mid = torch.cat([pad_l, patch, pad_r], dim=-1)
            patch_bot = torch.cat([pad_b_l, pad_b, pad_b_r], dim=-1)
            non_overlapped_patch = torch.cat([patch_top, patch_mid, patch_bot], dim=-2)
        else:
            patch_top = torch.cat([pad_t_l, pad_t], dim=-1)
            patch_mid = torch.cat([pad_l, patch], dim=-1)
            non_overlapped_patch = torch.cat([patch_top, patch_mid], dim=-2)

        return non_overlapped_patch

    def forward(self, x):
        non_overlapped_patch_6d =  self._forward_non_overlapping(x)
        non_overlapped_patch_4d = rearrange(non_overlapped_patch_6d, "B C ph pw H W -> (B ph pw) C H W", ph=self.ph, pw=self.pw)
        out = self.conv(non_overlapped_patch_4d)
        out = rearrange(out, "(B ph pw) C H W -> B C (ph H) (pw W)", ph=self.ph, pw=self.pw)
        if self.coefficient_train and self.training:
            return non_overlapped_patch_6d, out
        return out


def get_parent_module_by_name(model, name):
    attrs = name.split('.')
    submodule = model
    for attr in attrs[:-1]:
        submodule = getattr(submodule, attr)
    return submodule

def transform_with_non_overlapping(model, args):
    assert len(args.patch_list) > 0
    assert args.pivot_idx < len(args.patch_list)

    i = 0
    for name, mod in model.named_modules():
        if isinstance(mod, nn.Conv2d):
            kernel_size = mod.kernel_size[0]
            if kernel_size == 1 or args.patch_list[i] != 0:
                continue
            parent_mod = get_parent_module_by_name(model, name)
            replaced_mod = NonOverlappedConv2d(mod, args)
            mod_name = name.split('.')[-1]
            setattr(parent_mod, mod_name, replaced_mod)
            i += 1
            if i == len(args.patch_list):
                break

def main():
    from torchvision.models import mobilenet_v2
    model = mobilenet_v2(pretrained=True)
    change_model_list(model, 4, module_to_mapping, [0, 0, 0, 0, 0])
    from torchvision.datasets import FakeData
    from torch.utils.data import Dataset, DataLoader
    from torchvision import transforms
    from torch.optim.lr_scheduler import StepLR
    import torch.optim as optim

    preprocessing = transforms.Compose([
        transforms.CenterCrop(224),
        transforms.ToTensor(),
        transforms.Normalize(mean=[0.458, 0.456, 0.406],
                             std=[0.229, 0.224, 0.224])
    ])
    train_dataset = FakeData(1024, image_size=(3, 224, 224), num_classes=1000, transform=preprocessing)
    train_loader = DataLoader(train_dataset, batch_size=32, shuffle=True)
    model.with_feat = None
    model.eval()
    optimizer = optim.SGD(model.parameters(), lr=0.1)
    scheduler = StepLR(optimizer, step_size=1, gamma=0.98)

    for epoch in range(5):
        progressive = [0, 0, 0, 0, 0]
        progressive[epoch] = 1
        print(f"=============================== epoch = {epoch} =============================")
        show_status(model)
        optimizer.param_groups[0]["lr"] = 0.1
        print(scheduler.get_lr()[0])
        for batch_idx, (input, target) in enumerate(train_loader):
            input.cuda()
            target.cuda()
            model(input)
            scheduler.step()

if __name__ == "__main__":
    main()