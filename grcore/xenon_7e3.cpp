// This demonstrates the precion of the Xenon 7e3 EDRAM format.
// Just compile it by hand from the command line, it's not in any workspace.


#include <stdio.h>
#include <math.h>

int main() {
	for (int bias = -4; bias <= 4; bias++) {
		double prev = 0;
		for (int m=0; m<128; m++) {
			double val = m * pow(2.0,-9+bias);
			printf("bias%2d  %02x:%1x - %f  [%f]\n",bias,m,0,val,val-prev);
			prev = val;
		}
		for (int e=1; e<8; e++) {
			for (int m=0; m<128; m++) {
				double val = (m + 128) * pow(2.0,e-10+bias);
				printf("bias%2d  %02x:%1x - %f  [%f]\n",bias,m,e,val,val-prev);
				prev = val;
			}
		}
	}
}
