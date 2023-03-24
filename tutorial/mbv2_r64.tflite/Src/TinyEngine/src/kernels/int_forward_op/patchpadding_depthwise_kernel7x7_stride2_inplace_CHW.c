/* This file is automatically generated */
/* ----------------------------------------------------------------------
 * Project: TinyEngine
 * Title:   patchpadding_depthwise_kernel7x7_stride1_inplace_CHW.c
 * Description:  for sparse in-place 7x7 depth-wise convolution (HWC->CHW->HWC)
 *
 * Reference papers:
 *  - MCUNet: Tiny Deep Learning on IoT Device, NeurIPS 2020
 *  - MCUNetV2: Memory-Efficient Patch-based Inference for Tiny Deep Learning, NeurIPS 2021
 *  - MCUNetV3: On-Device Training Under 256KB Memory, NeurIPS 2022
 * Contact authors:
 *  - Wei-Ming Chen, wmchen@mit.edu
 *  - Wei-Chen Wang, wweichen@mit.edu
 *  - Ji Lin, jilin@mit.edu
 *  - Ligeng Zhu, ligeng@mit.edu
 *  - Song Han, songhan@mit.edu
 *
 * Target ISA:  ARMv7E-M
 * -------------------------------------------------------------------- */

#include "arm_nnsupportfunctions.h" //TODO: remove this in the future for self-contained
#include "tinyengine_function.h"
void patch_depthwise_kernel7x7_stride2_inplace_kernel_CHW(
		const uint16_t output_y, const uint16_t output_x,
		const int32_t *bias, const int32_t *biasR, const q7_t *ksrc, const int32_t *multiplier,
		const int32_t *shift, q7_t *output, const int32_t output_offset,
		const int32_t activation_min, const int32_t activation_max,
		q7_t *cols_8b_iterptr, const uint16_t column_x, int channel_offset);
tinyengine_status patchpadding_depthwise_kernel7x7_stride2_inplace_CHW(q7_t *input, const uint16_t input_x, const uint16_t input_y,
				const uint16_t input_ch, const q7_t *kernel, const int32_t *bias, const int32_t *biasR,
				const int32_t *output_shift, const int32_t *output_mult,
				const int32_t output_offset, const int32_t input_offset,
				const int32_t output_activation_min,
				const int32_t output_activation_max, q7_t *output,
				const uint16_t output_x, const uint16_t output_y,
				const uint16_t output_ch, q15_t *runtime_buf, q7_t pad_value,
				const uint16_t pad_t, const uint16_t pad_b, const uint16_t pad_l, const uint16_t pad_r)
{
    // adding pad_t, pad_b, pad_l, pad_r
    uint16_t c,i,j;
	q7_t *cols_8b_start = (q7_t *)runtime_buf;
	q7_t* cols_8b = (q7_t* )cols_8b_start;

	const q7_t *src;
	const q7_t *ksrc = kernel;

	//set the output for inplace update
	q7_t *inplace_out = input;

	int padding_cnt = pad_t * input_x;
	//shift the input ptr accordingly for HWC->CHW
	input += padding_cnt * input_ch;
	//handle top padding
	q7_t PAD8 = pad_value;
	while (padding_cnt--){
		*cols_8b++ = PAD8;
	}

	for (i = pad_t; i < input_y - pad_b; i++){
		//handle left padding
		for (j = 0; j < pad_l; j++){
			*cols_8b++ = PAD8;
		}
		cols_8b += input_x - (pad_l + pad_r);
		//handle right padding
		for (j = 0; j < pad_r; j++){
			*cols_8b++ = PAD8;
		}
	}

	//handle bottom padding
	padding_cnt = pad_b * input_x;
	//not need to shift for bottom padding
	while (padding_cnt--){
		*cols_8b++ = PAD8;
	}

	for (c = 0; c < input_ch; c++){        
	    src = input;
        cols_8b = (q7_t*)(cols_8b_start + pad_t * (input_x)); //skip pad_t rows
        for(i = pad_t; i < input_y - pad_b; i++){
            cols_8b += pad_l;//skip left
            src += pad_l * input_ch;
            for(j = pad_l; j < input_x - pad_r; j++){
                *cols_8b++ = *src;// + input_offset;
                src += input_ch;
            }
            cols_8b += pad_r;//skip right
            src += pad_r * input_ch;
        }
		patch_depthwise_kernel7x7_stride2_inplace_kernel_CHW(output_y, output_x, bias++, biasR++, ksrc, output_mult++, output_shift++, inplace_out, output_offset,output_activation_min, output_activation_max,cols_8b_start, input_x, input_ch);
		inplace_out++;
		input++;
		ksrc += 49;
    }
}
void patch_depthwise_kernel7x7_stride2_inplace_kernel_CHW(
		const uint16_t output_y, const uint16_t output_x,
		const int32_t *bias, const int32_t *biasR, const q7_t *ksrc, const int32_t *multiplier,
		const int32_t *shift, q7_t *output, const int32_t output_offset,
		const int32_t activation_min, const int32_t activation_max,
		q7_t *cols_8b_iterptr, const uint16_t column_x, int channel_offset)
{
	#define STRIDE 2
	int i, j;
	/* MACs for each output */
	for (i = 0; i < output_y; i++) {
		for (j = 0; j < output_x / 2; j++) {
			q7_t *cols_8b = cols_8b_iterptr;

			q31_t sum0 = bias[0];
			q31_t sum1 = bias[0];

           /* computation */
            sum0 += cols_8b[0]*ksrc[0];
            sum1 += cols_8b[1]*ksrc[0];
            sum0 += cols_8b[1]*ksrc[1];
            sum1 += cols_8b[2]*ksrc[1];
            sum0 += cols_8b[2]*ksrc[2];
            sum1 += cols_8b[3]*ksrc[2];
            sum0 += cols_8b[3]*ksrc[3];
            sum1 += cols_8b[4]*ksrc[3];
            sum0 += cols_8b[4]*ksrc[4];
            sum1 += cols_8b[5]*ksrc[4];
            sum0 += cols_8b[5]*ksrc[5];
            sum1 += cols_8b[6]*ksrc[5];
            sum0 += cols_8b[6]*ksrc[6];
            sum1 += cols_8b[7]*ksrc[6];
            cols_8b += column_x;
            sum0 += cols_8b[0]*ksrc[7];
            sum1 += cols_8b[1]*ksrc[7];
            sum0 += cols_8b[1]*ksrc[8];
            sum1 += cols_8b[2]*ksrc[8];
            sum0 += cols_8b[2]*ksrc[9];
            sum1 += cols_8b[3]*ksrc[9];
            sum0 += cols_8b[3]*ksrc[10];
            sum1 += cols_8b[4]*ksrc[10];
            sum0 += cols_8b[4]*ksrc[11];
            sum1 += cols_8b[5]*ksrc[11];
            sum0 += cols_8b[5]*ksrc[12];
            sum1 += cols_8b[6]*ksrc[12];
            sum0 += cols_8b[6]*ksrc[13];
            sum1 += cols_8b[7]*ksrc[13];
            cols_8b += column_x;
            sum0 += cols_8b[0]*ksrc[14];
            sum1 += cols_8b[1]*ksrc[14];
            sum0 += cols_8b[1]*ksrc[15];
            sum1 += cols_8b[2]*ksrc[15];
            sum0 += cols_8b[2]*ksrc[16];
            sum1 += cols_8b[3]*ksrc[16];
            sum0 += cols_8b[3]*ksrc[17];
            sum1 += cols_8b[4]*ksrc[17];
            sum0 += cols_8b[4]*ksrc[18];
            sum1 += cols_8b[5]*ksrc[18];
            sum0 += cols_8b[5]*ksrc[19];
            sum1 += cols_8b[6]*ksrc[19];
            sum0 += cols_8b[6]*ksrc[20];
            sum1 += cols_8b[7]*ksrc[20];
            cols_8b += column_x;
            sum0 += cols_8b[0]*ksrc[21];
            sum1 += cols_8b[1]*ksrc[21];
            sum0 += cols_8b[1]*ksrc[22];
            sum1 += cols_8b[2]*ksrc[22];
            sum0 += cols_8b[2]*ksrc[23];
            sum1 += cols_8b[3]*ksrc[23];
            sum0 += cols_8b[3]*ksrc[24];
            sum1 += cols_8b[4]*ksrc[24];
            sum0 += cols_8b[4]*ksrc[25];
            sum1 += cols_8b[5]*ksrc[25];
            sum0 += cols_8b[5]*ksrc[26];
            sum1 += cols_8b[6]*ksrc[26];
            sum0 += cols_8b[6]*ksrc[27];
            sum1 += cols_8b[7]*ksrc[27];
            cols_8b += column_x;
            sum0 += cols_8b[0]*ksrc[28];
            sum1 += cols_8b[1]*ksrc[28];
            sum0 += cols_8b[1]*ksrc[29];
            sum1 += cols_8b[2]*ksrc[29];
            sum0 += cols_8b[2]*ksrc[30];
            sum1 += cols_8b[3]*ksrc[30];
            sum0 += cols_8b[3]*ksrc[31];
            sum1 += cols_8b[4]*ksrc[31];
            sum0 += cols_8b[4]*ksrc[32];
            sum1 += cols_8b[5]*ksrc[32];
            sum0 += cols_8b[5]*ksrc[33];
            sum1 += cols_8b[6]*ksrc[33];
            sum0 += cols_8b[6]*ksrc[34];
            sum1 += cols_8b[7]*ksrc[34];
            cols_8b += column_x;
            sum0 += cols_8b[0]*ksrc[35];
            sum1 += cols_8b[1]*ksrc[35];
            sum0 += cols_8b[1]*ksrc[36];
            sum1 += cols_8b[2]*ksrc[36];
            sum0 += cols_8b[2]*ksrc[37];
            sum1 += cols_8b[3]*ksrc[37];
            sum0 += cols_8b[3]*ksrc[38];
            sum1 += cols_8b[4]*ksrc[38];
            sum0 += cols_8b[4]*ksrc[39];
            sum1 += cols_8b[5]*ksrc[39];
            sum0 += cols_8b[5]*ksrc[40];
            sum1 += cols_8b[6]*ksrc[40];
            sum0 += cols_8b[6]*ksrc[41];
            sum1 += cols_8b[7]*ksrc[41];
            cols_8b += column_x;
            sum0 += cols_8b[0]*ksrc[42];
            sum1 += cols_8b[1]*ksrc[42];
            sum0 += cols_8b[1]*ksrc[43];
            sum1 += cols_8b[2]*ksrc[43];
            sum0 += cols_8b[2]*ksrc[44];
            sum1 += cols_8b[3]*ksrc[44];
            sum0 += cols_8b[3]*ksrc[45];
            sum1 += cols_8b[4]*ksrc[45];
            sum0 += cols_8b[4]*ksrc[46];
            sum1 += cols_8b[5]*ksrc[46];
            sum0 += cols_8b[5]*ksrc[47];
            sum1 += cols_8b[6]*ksrc[47];
            sum0 += cols_8b[6]*ksrc[48];
            sum1 += cols_8b[7]*ksrc[48];

            /* requantize */
            sum0 = arm_nn_requantize(sum0 + biasR[0], *multiplier, *shift);
            sum0 += output_offset;
            sum0 = MAX(sum0, activation_min);
            sum0 = MIN(sum0, activation_max);
            output[(i * output_x + j * 2) * channel_offset] = sum0;

            sum1 = arm_nn_requantize(sum1 + biasR[0], *multiplier, *shift);
            sum1 += output_offset;
            sum1 = MAX(sum1, activation_min);
            sum1 = MIN(sum1, activation_max);
            output[(i * output_x + (j * 2 + 1)) * channel_offset] = sum1;

            cols_8b_iterptr += STRIDE * 2;
        }
        if (output_x & 1) {
            q7_t * cols_8b = cols_8b_iterptr;
            q31_t sum = bias[0] + biasR[0];
            sum += cols_8b[0]*ksrc[0];
            sum += cols_8b[1]*ksrc[1];
            sum += cols_8b[2]*ksrc[2];
            sum += cols_8b[3]*ksrc[3];
            sum += cols_8b[4]*ksrc[4];
            sum += cols_8b[5]*ksrc[5];
            sum += cols_8b[6]*ksrc[6];
            cols_8b += column_x;
            sum += cols_8b[0]*ksrc[7];
            sum += cols_8b[1]*ksrc[8];
            sum += cols_8b[2]*ksrc[9];
            sum += cols_8b[3]*ksrc[10];
            sum += cols_8b[4]*ksrc[11];
            sum += cols_8b[5]*ksrc[12];
            sum += cols_8b[6]*ksrc[13];
            cols_8b += column_x;
            sum += cols_8b[0]*ksrc[14];
            sum += cols_8b[1]*ksrc[15];
            sum += cols_8b[2]*ksrc[16];
            sum += cols_8b[3]*ksrc[17];
            sum += cols_8b[4]*ksrc[18];
            sum += cols_8b[5]*ksrc[19];
            sum += cols_8b[6]*ksrc[20];
            cols_8b += column_x;
            sum += cols_8b[0]*ksrc[21];
            sum += cols_8b[1]*ksrc[22];
            sum += cols_8b[2]*ksrc[23];
            sum += cols_8b[3]*ksrc[24];
            sum += cols_8b[4]*ksrc[25];
            sum += cols_8b[5]*ksrc[26];
            sum += cols_8b[6]*ksrc[27];
            cols_8b += column_x;
            sum += cols_8b[0]*ksrc[28];
            sum += cols_8b[1]*ksrc[29];
            sum += cols_8b[2]*ksrc[30];
            sum += cols_8b[3]*ksrc[31];
            sum += cols_8b[4]*ksrc[32];
            sum += cols_8b[5]*ksrc[33];
            sum += cols_8b[6]*ksrc[34];
            cols_8b += column_x;
            sum += cols_8b[0]*ksrc[35];
            sum += cols_8b[1]*ksrc[36];
            sum += cols_8b[2]*ksrc[37];
            sum += cols_8b[3]*ksrc[38];
            sum += cols_8b[4]*ksrc[39];
            sum += cols_8b[5]*ksrc[40];
            sum += cols_8b[6]*ksrc[41];
            cols_8b += column_x;
            sum += cols_8b[0]*ksrc[42];
            sum += cols_8b[1]*ksrc[43];
            sum += cols_8b[2]*ksrc[44];
            sum += cols_8b[3]*ksrc[45];
            sum += cols_8b[4]*ksrc[46];
            sum += cols_8b[5]*ksrc[47];
            sum += cols_8b[6]*ksrc[48];

            sum = arm_nn_requantize(sum + biasR[0], *multiplier, *shift);
            sum += output_offset;
            sum = MAX(sum, activation_min);
            sum = MIN(sum, activation_max);
            output[(i * output_x + output_x - 1) * channel_offset] = sum;

            cols_8b_iterptr += STRIDE * 2;
        }
        cols_8b_iterptr += 3 * 2;
    }
}

