#include <stdio.h>
#include <math.h>
#include <string.h>
#include <fftw3.h>

#define NTSC_BLOCK_SIZE 800
#define PAL_BLOCK_SIZE 960
#define FFT_BLOCK_SIZE 1024
#define MAX_SIZE 255.0

static double max = 0.0;

void fft_block(double *data)

{
  fftw_complex *in, *out;
  fftw_plan p;
  int loop;

  in = fftw_malloc(sizeof(fftw_complex) * FFT_BLOCK_SIZE);
  out = fftw_malloc(sizeof(fftw_complex) * FFT_BLOCK_SIZE);
  for(loop = 0; loop < FFT_BLOCK_SIZE; loop++)
    {
      in[loop][0] = data[loop];
      in[loop][1] = 0.0;
    }

  p = fftw_plan_dft_1d(FFT_BLOCK_SIZE, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

  fftw_execute(p); /* repeat as needed */
 
  fftw_destroy_plan(p);

  for(loop = 0; loop < FFT_BLOCK_SIZE; loop++)
    {
      data[loop] = sqrt(out[loop][0] * out[loop][0] + out[loop][1] * out[loop][1]);
    }
  
  fftw_free(in); fftw_free(out);
}

/* Interpolate block to 1024 samples */
void int_block(double *out, const unsigned short *in, int block_size)

{
  int pos;
  double block_pos;
  unsigned int low, high;
  double offset;

  out[0] = (double) in[0];
  out[FFT_BLOCK_SIZE-1] = (double) in[block_size-1];
  for(pos = 1; pos < FFT_BLOCK_SIZE-1; pos++)
    {
      block_pos = (double) pos;
      block_pos /= (double) FFT_BLOCK_SIZE;
      block_pos *= (double) block_size;

      low = (unsigned int) floor(block_pos);
      high = (unsigned int) ceil(block_pos);

      offset = block_pos - floor(block_pos);
      out[pos] = offset * ((double) in[high] - (double) in[low]) + (double) in[low];
    }
}

double outbufs[2][FFT_BLOCK_SIZE];
unsigned short inbufs[2][FFT_BLOCK_SIZE];
unsigned short u16_block[FFT_BLOCK_SIZE];

void mix_block(double *left, double *right)

{
  int loop;

  for(loop = 0; loop < FFT_BLOCK_SIZE; loop++)
    {
      left[loop] += right[loop];
    }
}

void normalise_block(unsigned short *out, double *fft)

{
  int loop = 0;

  out[loop] = 0;
  for(loop = 1; loop < FFT_BLOCK_SIZE; loop++)
  {
     double temp;
     temp = fft[loop] / ((double) FFT_BLOCK_SIZE * 2.0);
     if(temp > max)
       max = temp;
     if(temp < 0) printf("Negative valuei\n");
     out[loop] = temp < 0 ? 0 : temp;
  }
}

void print_block_i(unsigned int *buf, int block_size)

{
  int loop;
  for(loop = 0; loop < block_size; loop++)
    {
      printf("%d, ", buf[loop]);
    }
  printf("\n");
}

void print_block_f(double *buf)

{
  int loop;
  
  for(loop = 0; loop < FFT_BLOCK_SIZE; loop++)
    {
      printf("%f, ", buf[loop]);
    }
  printf("\n");
}

int convert(const char *left, const char *right, const char *outfile, int block_size)

{
  FILE *fpl, *fpr, *fpout;
  long len;

  fpl = fopen(left, "rb");
  if(fpl == NULL)
    {
      printf("Couldn't open %s\n", left);
      return 1;
    }
  
  fpr = fopen(right, "rb");
  if(fpr == NULL)
    {
      fclose(fpl);
      printf("Couldn't open %s\n", right);
      return 1;
    }

  fseek(fpl, 0, SEEK_END);
  len = ftell(fpl);

  fseek(fpr, 0, SEEK_END);
  if(len != ftell(fpr))
    {
      fclose(fpl);
      fclose(fpr);
      printf("right and left channels must be equal in size\n");
      return 1;
    }

  fseek(fpl, 0, SEEK_SET);
  fseek(fpr, 0, SEEK_SET);

  fpout = fopen(outfile, "wb");
  if(fpout == NULL)
    {
      fclose(fpl);
      fclose(fpr);
      printf("Couldn't write to %s\n", outfile);
      return 1;
    }

  while(len > 0)
    {
      memset(inbufs[0], 0, FFT_BLOCK_SIZE * sizeof(unsigned short));
      memset(inbufs[1], 0, FFT_BLOCK_SIZE * sizeof(unsigned short));
      memset(outbufs[0], 0, FFT_BLOCK_SIZE * sizeof(double));
      memset(outbufs[1], 0, FFT_BLOCK_SIZE * sizeof(double));

      fread(inbufs[0], sizeof(unsigned short), block_size, fpl);
      fread(inbufs[1], sizeof(unsigned short), block_size, fpr);

      int_block(outbufs[0], inbufs[0], block_size);
      int_block(outbufs[1], inbufs[1], block_size);
      fft_block(outbufs[0]);
      fft_block(outbufs[1]);
      mix_block(outbufs[0], outbufs[1]);
      normalise_block(u16_block, outbufs[0]);
      fwrite(u16_block, sizeof(unsigned short), FFT_BLOCK_SIZE, fpout);
      len -= block_size*sizeof(unsigned short);
    }

  fclose(fpl);
  fclose(fpr);
  fclose(fpout);
  printf("Maximum value = %f\n", max);

  return 0;
}

int main(int argc, char **argv)

{
  if(argc < 3)
    {
      printf("Usage fft_gen left right\n");
      return 1;
    }

  convert(argv[1], argv[2], "pal.fft", PAL_BLOCK_SIZE);
  convert(argv[1], argv[2], "ntsc.fft", NTSC_BLOCK_SIZE);
  
  return 0;
}
