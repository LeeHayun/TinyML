#./distributed_train.sh 2 /data/imagenet  --model efficientnet_b2 -b 128 --sched cosine --epochs 450 --decay-epochs 2.4 --decay-rate .97 --opt sgd --opt-eps .001 -j 8 --warmup-lr 1e-6 --weight-decay 1e-5 --drop 0.3 --drop-connect 0.2 --model-ema --output output/train/resnet --amp

torchrun --standalone --nnodes 1 --nproc_per_node 2 --master_port 12346 --node_rank 2 alter_train1.py /data/imagenet --model mobilenetv2_tiny_zero -b 1024 --sched cosine  --epochs 20 --lr-base 1.25e-4 --input-size 3 144 144 --weight-decay 4e-5 --momentum 0.9 --smoothing 0.1 -j 8 --seed 42 --opt sgd --output ./output/train/interpolate_00000_v9_auto_fine_weight_lr --log-wandb --resume="./output/train/interpolate_00000_reg_v1/mobilenetv2_w35_/model_best.pth.tar" --experiment mobilenetv2_w35_ --warmup-epochs 0 --pin-mem --grad-accum-steps 1 --num-patches=3 --patch-list 0 0 0 0 0

#torchrun --standalone --nnodes 1 --nproc_per_node 2 --master_port 12345 --node_rank 4 train.py /data/imagenet --model mobilenetv2_non_replicate -b 128 --sched step --decay-rate 0.98 --decay-epochs 1.0 --epochs 300 --lr-base 0.045 --input-size 3 224 224 --weight-decay 4e-5 --momentum 0.9 --smoothing 0.0 -j 8 --seed 42 --opt sgd --output ./output/train/mobilenetv2_non_reflect --experiment patchpadding --warmup-epochs 0 --local_rank 4 --log-wandb
