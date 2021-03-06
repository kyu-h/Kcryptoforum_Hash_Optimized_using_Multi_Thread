#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "FileOut_struct.h"

void Keccak(int rate, int capacity, const unsigned char *input, unsigned long long int inputByteLen, unsigned char delimitedSuffix, unsigned char *output, unsigned long long int outputByteLen);

typedef unsigned char BitSequence;
typedef size_t BitLength;
typedef enum { SUCCESS = 0, FAIL = 1, BAD_HASHLEN = 2 } HashReturn;

#define MAX_MARKER_LEN      4096
#define SUBMITTER_INFO_LEN  128

typedef enum { KAT_SUCCESS = 0, KAT_FILE_OPEN_ERROR = 1, KAT_HEADER_ERROR = 2, KAT_DATA_ERROR = 3, KAT_HASH_ERROR = 4 } STATUS_CODES;

#define ExcludeExtremelyLong

#define SqueezingOutputLength 4096

STATUS_CODES    genShortMsgHash(unsigned int rate, unsigned int capacity, unsigned char delimitedSuffix, unsigned int hashbitlen, unsigned int squeezedOutputLength, const char *inputFileName, const char *outputFileName, const char *description);
int     FindMarker(FILE *infile, const char *marker);
void    fprintBstr(FILE *fp, char *S, BitSequence *A, int L);
void convertShortMsgToPureLSB(void);

STATUS_CODES
genKAT_main(void)
{
    /* The following instances are from the FIPS 202 standard. */
    /* http://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.202.pdf */
    /*  */
    /* Note: "SakuraSequential" translates into "input followed by 11", */
    /* see https://keccak.team/files/Sakura.pdf for more details. */
    /*  */

	FILE *fp_in;
	char strTemp[255];
	char *pStr;
	char *HashName[4] = {"Hash_DRBG_SHA3-224", "Hash_DRBG_SHA3-256", "Hash_DRBG_SHA3-384", "Hash_DRBG_SHA3-512"};
	char inputFileAddress[256], outputFileAddress[256];

	for(int i=0; i<4; i++){
		sprintf(inputFileAddress, "Hash_testvectors/%s.txt", HashName[i]);
		sprintf(outputFileAddress, "Hash_testvectors/%s(no PR)_rsp.txt", HashName[i]);

		if ( (fp_in = fopen(inputFileAddress, "r")) == NULL ) {
			printf("Couldn't open <%s> for read\n", inputFileAddress);
			return KAT_FILE_OPEN_ERROR;
		}

		pStr = fgets(strTemp, sizeof(strTemp), fp_in);
		printf("%s", pStr);

		if(!strcmp(pStr, "Alg_ID = Hash_DRBG_SHA3-224\n")){
			//genShortMsgHash(1152, 448, 0x06, 224, 0,inputFileAddress,outputFileAddress,"Alg_ID = Hash_DRBG_SHA3-224");
		}else if(!strcmp(pStr, "Alg_ID = Hash_DRBG_SHA3-256\n")){
			//genShortMsgHash(1088, 512, 0x06, 256, 0,inputFileAddress,outputFileAddress,"Alg_ID = Hash_DRBG_SHA3-256");
		}else if(!strcmp(pStr, "Alg_ID = Hash_DRBG_SHA3-384\n")){
			//genShortMsgHash(832, 768, 0x06, 384, 0,inputFileAddress,outputFileAddress,"Alg_ID = Hash_DRBG_SHA3-384");
		}else if(!strcmp(pStr, "Alg_ID = Hash_DRBG_SHA3-512\n")){
			genShortMsgHash(576, 1024, 0x06, 512, 0,inputFileAddress,outputFileAddress,"Alg_ID = Hash_DRBG_SHA3-512");
		}else {
			printf("Error!\n");
		}
	}

	fclose(fp_in);
    return KAT_SUCCESS;
}

STATUS_CODES
genShortMsgHash(unsigned int rate, unsigned int capacity, unsigned char delimitedSuffix, unsigned int hashbitlen, unsigned int squeezedOutputLength, const char *inputFileName, const char *outputFileName, const char *description)
{
    struct DRBG_SHA3 ctx;
	BitSequence Squeezed[SqueezingOutputLength/8];
    FILE *fp_in, *fp_out;
    char string[1000001] = {0, };
    char str;
    int nCount=0;

    BitSequence entropy01[hashbitlen], entropy02[hashbitlen], entropy03[hashbitlen], nonce[hashbitlen/2], perString[hashbitlen], addinput01[hashbitlen], addinput02[hashbitlen];
    BitSequence Hex_entropy01[hashbitlen];
    unsigned int outputLen = capacity;
    int r, w, a, b, c, d, e, f = 0;

    if ((squeezedOutputLength > SqueezingOutputLength) || (hashbitlen > SqueezingOutputLength)) {
		printf("Requested output length too long.\n");
		return KAT_HASH_ERROR;
	}

	if ( (fp_in = fopen(inputFileName, "r")) == NULL ) {
		printf("Couldn't open <ShortMsgKAT.txt> for read\n");
		return KAT_FILE_OPEN_ERROR;
	}

	fp_out = fopen(outputFileName, "w");

	ctx.file_output = fp_out;
	fprintf(fp_out, "%s\n\n", description);

	FindMarker(fp_in, "entropy1");
	fscanf(fp_in, " %c %s", &str, &entropy01);

	FindMarker(fp_in, "entropy2");
	fscanf(fp_in, " %c %s", &str, &entropy02);

	FindMarker(fp_in, "entropy3");
	fscanf(fp_in, " %c %s", &str, &entropy03);

	FindMarker(fp_in, "nonce");
	fscanf(fp_in, " %c %s", &str, &nonce);

	FindMarker(fp_in, "perString");
	fscanf(fp_in, " %c %s", &str, &perString);

	FindMarker(fp_in, "addinput1");
	fscanf(fp_in, " %c %s", &str, &addinput01);

	FindMarker(fp_in, "addinput2");
	fscanf(fp_in, " %c %s", &str, &addinput02);

	for(int i=0; i<strlen(entropy01); i++){
		if(entropy01[i] >='A' && entropy01[i] <= 'Z'){
			entropy01[i] = entropy01[i] +32;
		}
		if(perString[i] >='A' && perString[i] <= 'Z'){
			perString[i] = perString[i] +32;
		}
	}

	for(int i=0; i<strlen(nonce); i++){
		if(nonce[i] >='A' && nonce[i] <= 'Z'){
			nonce[i] = nonce[i] +32;
		}
	}

	fprintf(fp_out, "entropy = %s\n", entropy01);
	fprintf(fp_out, "nonce = %s\n", nonce);
	fprintf(fp_out, "perString = %s\n\n", perString);

	for(r = 0, w =0, a = 0, b=0, c=0, d=0, e=0; r < 64 ; r += 2){
		unsigned char temp_arr[3] = {entropy01[r], entropy01[r+1], '\0'};
		entropy01[w++] = strtol(temp_arr, NULL, 16);

		unsigned char temp_arr01[3] = {entropy02[r], entropy02[r+1], '\0'};
		entropy02[a++] = strtol(temp_arr01, NULL, 16);

		unsigned char temp_arr02[3] = {entropy03[r], entropy03[r+1], '\0'};
		entropy03[b++] = strtol(temp_arr02, NULL, 16);

		unsigned char temp_arr03[3] = {perString[r], perString[r+1], '\0'};
		perString[c++] = strtol(temp_arr03, NULL, 16);

		unsigned char temp_arr04[3] = {addinput01[r], addinput01[r+1], '\0'};
		addinput01[d++] = strtol(temp_arr04, NULL, 16);

		unsigned char temp_arr05[3] = {addinput02[r], addinput02[r+1], '\0'};
		addinput02[e++] = strtol(temp_arr05, NULL, 16);

	} //2 string to hex

	for(r=0, f=0; f<64; r+=2){
		unsigned char tmp_arr[3] = {nonce[r], nonce[r+1], '\0'};
		nonce[f++] = strtol(tmp_arr, NULL, 16);
	}

	ResetFunction(&ctx, rate, capacity, delimitedSuffix, Squeezed, hashbitlen, entropy01, nonce, perString, addinput01);

	fprintf(fp_out, "addInput = ");
	for(int i=0; i<e; i++){
		fprintf(fp_out, "%02x", addinput02[i]);
	}
	fprintf(fp_out, "\n\n");

	fprintf(fp_out, "entropy = ");
	for(int i=0; i<a; i++){
		fprintf(fp_out, "%02x", entropy02[i]);
	}
	fprintf(fp_out, "\n\n");

	SecondResetFunction(&ctx, rate, capacity, delimitedSuffix, Squeezed, hashbitlen, entropy02, ctx.V_wCreseed, addinput02);

    printf("finished ShortMsgKAT for <%s>\n", inputFileName);

    fclose(fp_in);
    fclose(fp_out);

    return KAT_SUCCESS;
}

/*  */
/* ALLOW TO READ HEXADECIMAL ENTRY (KEYS, DATA, TEXT, etc.) */
/*  */
int FindMarker(FILE *infile, const char *marker){
    char    line[MAX_MARKER_LEN];
    int     i, len;

    len = (int)strlen(marker);
    if ( len > MAX_MARKER_LEN-1 )
        len = MAX_MARKER_LEN-1;

    for ( i=0; i<len; i++ )
        if ( (line[i] = fgetc(infile)) == EOF )
            return 0;
    line[len] = '\0';

    while ( 1 ) {
        if ( !strncmp(line, marker, len) )
            return 1;

        for ( i=0; i<len-1; i++ )
        	line[i] = line[i+1];

        if ( (line[len-1] = fgetc(infile)) == EOF )
			return 0;
        line[len] = '\0';
    }

    /* shouldn't get here */
    return 0;
}

void fprintBstr(FILE *fp, char *S, BitSequence *A, int L){
    int     i;

    fprintf(fp, "%s", S);

    for ( i=0; i<L; i++ )
        fprintf(fp, "%02x", A[i]); //write small

    if ( L == 0 )
        fprintf(fp, "00");

    fprintf(fp, "\n");
}

void operation_add(unsigned char *arr, int ary_size, int start_index, unsigned int num){
	unsigned int current;
	unsigned int carry = 0;
	start_index++;

	current = arr[ary_size - start_index];
	current += num;
	carry = (current >> 8);
	arr[ary_size - start_index] = (unsigned char) current;

    while(carry){
    	start_index++;
    	current = arr[ary_size - start_index];
		current += carry;
		carry = (current >> 8);
		arr[ary_size - start_index] = (unsigned char) current;
    }
}

void Output_Generation_Func(struct DRBG_SHA3 *ctx, unsigned int rate, unsigned int capacity, unsigned char delimitedSuffix, unsigned char *output, unsigned long long int outputByteLen, unsigned char *V, unsigned int Vlen, unsigned char *C, unsigned int Clen, unsigned char *addinput01){
	unsigned int reseed = 0x01;
	static int func_call = 0;
	BitSequence First_before_SHA3[10000];
	BitSequence First_after_SHA3[outputByteLen/8];
	BitSequence Second_before_SHA3[10000];
	BitSequence Second_after_SHA3[outputByteLen/8];
	BitSequence MiddleV[Vlen];

	BitSequence Squeezed[SqueezingOutputLength/8];
	int r, w, length = 0;

	printf("************Output_Generation_Function start************\n");
	printf("func_call : %d", func_call);

	//*********************buff**************************//


	if(func_call == 1){
		ctx->reseed_counter = reseed;
		fprintf(ctx->file_output, "reseed_counter = %d\n\n", ctx->reseed_counter);
	}else {
		First_before_SHA3[0] = 0x02;
		for(r = 0, w=1; r < Vlen; r++){ //inputByteLen를 v에 대한 길이로 변경
			First_before_SHA3[w++] = V[r];
		}

		length = strlen(addinput01) / 2;
		for(r=0; r<length; r++){
			First_before_SHA3[w++] = addinput01[r];
		}

		for(int i=0; i<length; i++){
			ctx->addInput[i] = addinput01[i];
		}
		ctx->addInput_length = length;

		//*********************buff**************************//
		printf("\nFirst before SHA3 data: ");
		for(int i=0; i<w; i++){
			printf("%02x", First_before_SHA3[i]);
		}
		printf("\n");

		Keccak(rate, capacity, First_before_SHA3, w, delimitedSuffix, Squeezed, outputByteLen/8); //have to check output, input length
		for (int i=0; i<outputByteLen/8; i++){
			First_after_SHA3[i] = Squeezed[i];
		}

		printf("First after SHA3 data: ");
		for(int i=0; i<outputByteLen/8; i++){
			printf("%02x", First_after_SHA3[i]);
		}
		printf("\n");

		for(int i=0; i<w; i++){
			ctx->W_VaddInput[i] = First_after_SHA3[i];
		}
		ctx->W_VaddInput_length = outputByteLen/8;

		for(int i = outputByteLen/8 - 1, start = 0 ; i > -1 ; i--){ //V + after sha3
			operation_add(V, Vlen, start++, First_after_SHA3[i]);
		}


		fprintf(ctx->file_output, "addInput = "); //=addInput1
		for(int i=0; i<length; i++){
			fprintf(ctx->file_output, "%02x", ctx->addInput[i]);
		}
		fprintf(ctx->file_output, "\n\n");

		fprintf(ctx->file_output, "w = "); //=Hash(0x02||V||addInput)
		for(int i=0; i<outputByteLen/8; i++){
			fprintf(ctx->file_output, "%02x", ctx->W_VaddInput[i]);
		}
		fprintf(ctx->file_output, "\n");

		fprintf(ctx->file_output, "V = "); //=(w + V) mod 2^440
		for(int i=0; i<Vlen; i++){
			fprintf(ctx->file_output, "%02x", V[i]);
		}
		fprintf(ctx->file_output, "\n\n");
	}

	printf("Middle V: ");
	for(int i=0; i<Vlen; i++){
		MiddleV[i] = V[i];
		printf("%02x", MiddleV[i]);
		ctx->V_Mod[i] = MiddleV[i];
	}
	ctx->V_Mod_length = Vlen;
	printf("\n");
	printf("Vlen : %d\n", Vlen);

	Inner_Output_Generation_Function(ctx, rate, capacity, MiddleV, Vlen, delimitedSuffix, Squeezed, outputByteLen/8); //have to check output, input length

	if(func_call == 0){
		fprintf(ctx->file_output, "output1 = "); //512-비트
	}else {
		fprintf(ctx->file_output, "output2 = "); //512-비트
	}

	for(int i=0; i<ctx->Output01_length; i++){ //change
		fprintf(ctx->file_output, "%02x", ctx->Output01[i]);
	}
	fprintf(ctx->file_output, "\n\n");

	//*********************buff01**************************//
	Second_before_SHA3[0] = 0x03;

	for(r = 0, w=1; r < Vlen; r++){
		Second_before_SHA3[w++] = V[r];
	} //2 string to hex
	//*********************buff01**************************//

	printf("Second before SHA3: ");
	for(int i=0; i<w; i++){
		printf("%02x", Second_before_SHA3[i]);
	}
	printf("\n");

	Keccak(rate, capacity, Second_before_SHA3, w, delimitedSuffix, Squeezed, outputByteLen/8); //have to check output, input length
	for (int i=0; i<outputByteLen/8; i++){
		Second_after_SHA3[i] = Squeezed[i];
		//printf("%02x", SHA3_values02[i]); //write small
	}

	printf("Second after SHA3: ");
	for(int i=0; i<outputByteLen/8; i++){
		printf("%02x", Second_after_SHA3[i]);
		ctx->W_03V[i] = Second_after_SHA3[i];
	}
	ctx->W_03V_length = outputByteLen/8;
	printf("\n");

	fprintf(ctx->file_output, "w = "); //=Hash(0x03||V)
	for(int i=0; i<ctx->W_03V_length; i++){
		fprintf(ctx->file_output, "%02x", ctx->W_03V[i]);
	}
	fprintf(ctx->file_output, "\n");

	printf("Final C: ");
	for(int i=0; i<Clen; i++){
		printf("%02x", C[i]);
	}
	printf("\n");

	//*********************sha3 + C + V + reseed**************************//
	for(int i = outputByteLen/8 - 1, start = 0 ; i > -1 ; i--){ //V + second after sha3
		operation_add(V, Vlen, start++, Second_after_SHA3[i]);
	}

	printf("V + after sha3: ");
	for(int i=0; i<Vlen; i++){
		printf("%02x", V[i]);
	}
	printf("\n");

	for(int i = Clen - 1, start = 0 ; i > -1 ; i--){ //V + C
		operation_add(V, Vlen, start++, C[i]);
	}

	printf("V + C: ");
	for(int i=0; i<Vlen; i++){
		printf("%02x", V[i]);
	}
	printf("\n");

	operation_add(V, Vlen, 0, reseed);

	printf("Final VV: ");
	for(int i=0; i<Vlen; i++){
		printf("%02x", V[i]);
		ctx->V_wCreseed[i] = V[i];
	}
	ctx->V_wCreseed_03V_length = Vlen;
	ctx->reseed_counter += 0x01;

	fprintf(ctx->file_output, "V = "); //=(w+V+C+reseed_counter) mod 2^440
	for(int i=0; i<ctx->V_wCreseed_03V_length; i++){
		fprintf(ctx->file_output, "%02x", ctx->V_wCreseed[i]);
	}
	fprintf(ctx->file_output, "\n");

	fprintf(ctx->file_output, "reseed_counter = ");
	fprintf(ctx->file_output, "%d", ctx->reseed_counter);
	fprintf(ctx->file_output, "\n\n");

	//tag kyu

	if(func_call == 0){
		fprintf(ctx->file_output, "V = ");
		for(int i=0; i<ctx->V_wCreseed_03V_length; i++){
			fprintf(ctx->file_output, "%02x", ctx->V_wCreseed[i]);
		}
		fprintf(ctx->file_output, "\n");

		fprintf(ctx->file_output, "C = ");
		for(int i=0; i<Clen; i++){
			fprintf(ctx->file_output, "%02x", C[i]);
		}
		fprintf(ctx->file_output, "\n");

		fprintf(ctx->file_output, "reseed_counter = ");
		fprintf(ctx->file_output, "%d", ctx->reseed_counter);
		fprintf(ctx->file_output, "\n");
		printf("\n");
	}

	func_call++;
}

void Inner_Output_Generation_Function(struct DRBG_SHA3 *ctx, unsigned int rate, unsigned int capacity, unsigned char *input, unsigned long long int inputByteLen, unsigned char delimitedSuffix, unsigned char *output, unsigned long long int outputByteLen){
	//int length = inputByteLen;
	static int func_call = 0;
	int BlockSize = 0;
	BitSequence SHA3_mod01[outputByteLen];
	BitSequence Final_SHA3_after_mod[outputByteLen * 3];
	int num= 0;
	int ModLen=0;
	int outputLen = 0;
	double k=0;
	BitSequence Squeezed[SqueezingOutputLength/8];

	if(rate == 1152){
		ModLen = 55;
		BlockSize = 224;
	}else if(rate == 1088) {
		ModLen = 55;
		BlockSize = 256;
	}else if(rate == 832) {
		ModLen = 111;
		BlockSize = 384;
	}else{
		ModLen = 111;
		BlockSize = 512;
	}

	outputLen = BlockSize * 2;

	k = ceil((double)outputLen / (double) BlockSize);

	printf("k: %f", k);

	printf("************Inner_Output_Generation_Function start************\n");

	printf("outputByteLen: %d\n", outputByteLen);

	for(int addinput01=0; addinput01< (int) k; addinput01++){
		for(int i=0; i<ModLen; i++){
			printf("%02x", input[i]);
		}printf("\n");

		printf("SHA3_mod01: ");
		Keccak(rate, capacity, input, ModLen, delimitedSuffix, Squeezed, outputByteLen);
		for (int i=0; i<outputByteLen; i++){
			SHA3_mod01[i] = Squeezed[i];
			//Final_SHA3_after_mod[num++] = SHA3_mod01[i];
			printf("%02x", SHA3_mod01[i]); //write small
		}
		printf("\n");

		for(int i=0; i<outputByteLen; i++){
			Final_SHA3_after_mod[num++] = SHA3_mod01[i];
		}

		operation_add(input, ModLen, 0, 0x01);
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!inner_output: %d\n", addinput01);
	}

	printf("Final_SHA3_after_mod: ");
	for (int i=0; i<outputByteLen * 2; i++){
		printf("%02x", Final_SHA3_after_mod[i]); //write small
		ctx->Output01[i] = Final_SHA3_after_mod[i];
	}
	ctx->Output01_length = outputByteLen * 2;

	printf("\n");
	func_call++;
}

void C_DerivedFunction(struct DRBG_SHA3 *ctx,unsigned int rate, unsigned int capacity, unsigned char input[110], unsigned long long int inputByteLen, unsigned char delimitedSuffix, unsigned char *output, unsigned long long int outputByteLen, unsigned char *addinput01) {
	unsigned char reseed= 0x01;
	BitSequence V[inputByteLen];
	BitSequence Key_values01[10000];
	BitSequence Key_values02[10000];
	BitSequence Key_values03[10000];
	BitSequence SHA3_values01[10000];
	BitSequence SHA3_values02[10000];
	BitSequence SHA3_values03[10000];
	BitSequence Add_Key[30000];
	BitSequence Input_data[10000];
	BitSequence Final_Key[110];
	BitSequence Squeezed[SqueezingOutputLength/8];

	int r, w, j = 0;

	for(int i=0; i<inputByteLen; i++){
		V[i] = input[i];
		printf("%02x", input[i]);
	}

	printf("************C_DerivedFunction start************\n");
	//*********************buff**************************//
	Input_data[5] = 0x00;

	for(r = 0, w=6; r < inputByteLen; r++){
		Input_data[w++] = input[r];
	} //2 string to hex

	for(int i=0; i<w; i++){
		ctx->dfInput01[i] = Input_data[i];
	}printf("\n");

	fprintf(ctx->file_output, "dfInput = "); //=0x00||V
	for(int i=5; i<w; i++){
		fprintf(ctx->file_output, "%02x", ctx->dfInput01[i]);
	}
	fprintf(ctx->file_output, "\n");
	Input_data[0] = 0x01;

	if(inputByteLen == 55) {
		Input_data[1] = 0x00;
		Input_data[2] = 0x00;
		Input_data[3] = 0x01;
		Input_data[4] = 0xB8;
	} else { //384, 512
		Input_data[1] = 0x00;
		Input_data[2] = 0x00;
		Input_data[3] = 0x03;
		Input_data[4] = 0x78;
	}

	for(r = 0, w =0; r < inputByteLen+6 ; r++){
		Key_values01[w++] = Input_data[r];
	}

	//*********************buff01**************************//

	printf("Key_values01: ");
	for(int i=0; i<w; i++){
		printf("%02x", Key_values01[i]);
	}printf("\n");

	Keccak(rate, capacity, Key_values01, w, delimitedSuffix, Squeezed, outputByteLen/8);

	for (int i=0; i<outputByteLen/8; i++){
		SHA3_values01[i] = Squeezed[i];
	}

	Input_data[0] = 0x02;

	//*********************buff02**************************//
	for(r = 0, w = 0; r < inputByteLen+6 ; r++){
		Key_values02[w++] = Input_data[r];
	} //2 string to hex
	//*********************buff02**************************//

	Keccak(rate, capacity, Key_values02, w, delimitedSuffix, Squeezed, outputByteLen/8);

	for (int i=0; i<outputByteLen/8; i++){
		SHA3_values02[i] = Squeezed[i];
	}

	if(rate == 832){
		Input_data[0] = 0x03;

		//*********************buff02**************************//
		for(r = 0, w = 0; r < inputByteLen+6 ; r++){
			Key_values03[w++] = Input_data[r];
		} //2 string to hex
		//*********************buff02**************************//

		Keccak(rate, capacity, Key_values03, w, delimitedSuffix, Squeezed, outputByteLen/8);

		for (int i=0; i<outputByteLen/8; i++){
			SHA3_values03[i] = Squeezed[i];
		}
	}


	for(int i=0; i< outputByteLen/8; i++){
		Add_Key[i] = SHA3_values01[i];
	}

	j = outputByteLen/8;
	for(int i=0; i< outputByteLen/8; i++){
		Add_Key[j++] = SHA3_values02[i];
	}

	if(rate == 832){
		for(int i=0; i< outputByteLen/8; i++){
			Add_Key[j++] = SHA3_values03[i];
		}
	}

	printf("C Final Key: ");
	for(int i=0; i<inputByteLen; i++){
		Final_Key[i] = Add_Key[i];
		printf("%02X", Final_Key[i]);
		ctx->dfOutput01[i] = Final_Key[i];
	}
	printf("\n");

	fprintf(ctx->file_output, "dfOutput = "); //=C
	for(int i=0; i<inputByteLen; i++){
		fprintf(ctx->file_output, "%02x", Final_Key[i]);
	}
	fprintf(ctx->file_output, "\n\n");

	if(ctx->reseed_counter < 2){
		//tag kyu
		fprintf(ctx->file_output, "V = "); //V
		for(int i=0; i<inputByteLen; i++){
			fprintf(ctx->file_output, "%02x", ctx->dfOutput[i]);
		}
		fprintf(ctx->file_output, "\n");

		fprintf(ctx->file_output, "C = ");
		for(int i=0; i<inputByteLen; i++){
			fprintf(ctx->file_output, "%02x", ctx->dfOutput01[i]);
		}
		fprintf(ctx->file_output, "\n");

		ctx->reseed_counter = reseed;
		fprintf(ctx->file_output, "reseed_counter = ");
		fprintf(ctx->file_output, "%d", ctx->reseed_counter);
		fprintf(ctx->file_output, "\n");
	}

	Output_Generation_Func(ctx, rate, capacity, delimitedSuffix, Squeezed, outputByteLen, V, inputByteLen, Final_Key, inputByteLen, addinput01);
}

void V_DerivedFunction(struct DRBG_SHA3 *ctx, unsigned int rate, unsigned int capacity, const unsigned char *input, unsigned long long int inputByteLen, unsigned char delimitedSuffix, unsigned char *output, unsigned long long int outputByteLen, unsigned char *addinput01) {
	BitSequence buff_01[100] = "01000001B8";
	BitSequence buff_02[100] = "02000001B8";
	BitSequence buff_03[100] = "0300000378";
	BitSequence Key_values01[10000];
	BitSequence Key_values02[10000];
	BitSequence Key_values03[10000];

	BitSequence SHA3_values01[10000];
	BitSequence SHA3_values02[10000];
	BitSequence SHA3_values03[10000];
	BitSequence Add_Key[30000];
	BitSequence Final_Key[110];
    BitSequence Squeezed[SqueezingOutputLength/8];
    int r, w, j = 0;
    int Vlen = 0;

    if(rate == 1152 || rate == 1088){
		Vlen = 55; //Seed_bit
	}else {
		Vlen = 111; //Seed_bit
		buff_01[7] = '3';
		buff_02[7] = '3';
		buff_01[8] = '7';
		buff_02[8] = '7';
		buff_01[9] = '8';
		buff_02[9] = '8';
	}

    printf("\n************V_DerivedFunction start************\n");

    //*********************buff01**************************//
    for(r = 0, w = 0 ; r < strlen(buff_01) ; r += 2){
        unsigned char temp_arr[3] = {buff_01[r], buff_01[r+1], '\0'};
        Key_values01[w++] = strtol(temp_arr, NULL, 16);
    } //2 string to hex

    for(int i=0; i<inputByteLen; i++){
    	Key_values01[w++] = input[i];
    }

    for(int i=0; i<w; i++){
    	printf("%02x", Key_values01[i]);
    }printf("\n");
    //*********************buff01**************************//

    Keccak(rate, capacity, Key_values01, w, delimitedSuffix, Squeezed, outputByteLen/8);
	for (int i=0; i<outputByteLen/8; i++){
		SHA3_values01[i] = Squeezed[i];
		printf("%02x", SHA3_values01[i]);
	}

    //*********************buff02**************************//
    for(r = 0, w = 0 ; r < strlen(buff_02) ; r += 2){
        unsigned char temp_arr[3] = {buff_02[r], buff_02[r+1], '\0'};
        Key_values02[w++] = strtol(temp_arr, NULL, 16);
    } //2 string to hex

    for(int i=0; i<inputByteLen; i++){
    	Key_values02[w++] = input[i];
	}
    printf("\ntttt\n");
    for(int i=0; i<w; i++){
		printf("%02x", Key_values02[i]);
    }printf("\n");
    //*********************buff02**************************//

	Keccak(rate, capacity, Key_values02, w, delimitedSuffix, Squeezed, outputByteLen/8);
	for (int i=0; i<outputByteLen/8; i++){
		SHA3_values02[i] = Squeezed[i];
	}

	if(rate == 832){
		for(r = 0, w = 0 ; r < strlen(buff_03) ; r += 2){
			unsigned char temp_arr[3] = {buff_03[r], buff_03[r+1], '\0'};
			Key_values03[w++] = strtol(temp_arr, NULL, 16);
		} //2 string to hex

		for(int i=0; i<inputByteLen; i++){
			Key_values03[w++] = input[i];
		}
		printf("\ntttt\n");
		for(int i=0; i<w; i++){
			printf("%02x", Key_values03[i]);
		}printf("\n");
		//*********************buff02**************************//

		Keccak(rate, capacity, Key_values03, w, delimitedSuffix, Squeezed, outputByteLen/8);
		for (int i=0; i<outputByteLen/8; i++){
			SHA3_values03[i] = Squeezed[i];
		}
	}

	for(int i=0; i< outputByteLen/8; i++){
		Add_Key[i] = SHA3_values01[i];
	}

	j = outputByteLen/8;
	for(int i=0; i< outputByteLen/8; i++){
		Add_Key[j++] = SHA3_values02[i];
	}

	if(rate == 832){
		for(int i=0; i< outputByteLen/8; i++){
			Add_Key[j++] = SHA3_values03[i];
		}
	}

    printf("\nV Final Key: ");
    for(int i=0; i<Vlen; i++){ //55맞는지 확인 필요
        Final_Key[i] = Add_Key[i];
        printf("%02X", Final_Key[i]);
        ctx->dfOutput[i] = Add_Key[i];
    }

    ctx->Vlen = Vlen;
    printf("\n");

	fprintf(ctx->file_output, "dfOutput = "); //=V
	for(int i=0; i<Vlen; i++){
		fprintf(ctx->file_output, "%02x", Final_Key[i]);
	}
	fprintf(ctx->file_output, "\n\n");

    C_DerivedFunction(ctx, rate, capacity, Final_Key, Vlen, delimitedSuffix, Squeezed, outputByteLen, addinput01);
}

void SecondResetFunction(struct DRBG_SHA3 *ctx, unsigned int rate, unsigned int capacity, unsigned char delimitedSuffix, unsigned char *output, unsigned long long int outputByteLen, unsigned char *entropy, unsigned char *V, unsigned char *addinput02){
	int num = 0;
	BitSequence buff[10] = "01";
	BitSequence input_data[1000];
	int entropyLen = 32;
	BitSequence Squeezed[SqueezingOutputLength/8];
	int Vlen, r, w = 0;

	printf("*********************SecondResetFunction**********************\n");

	if(rate == 1152 || rate == 1088){
		Vlen = 55;
	}else {
		Vlen = 111; //seed_length
	}

	for(r = 0, w = 0 ; r < strlen(buff) ; r += 2){
		unsigned char temp_arr[3] = {buff[r], buff[r+1], '\0'};
		input_data[num++] = strtol(temp_arr, NULL, 16);
	} //2 string to hex

	for(int i=0; i<Vlen; i++){
		input_data[num++] = V[i];
	}

	for(int i=0; i<entropyLen; i++){
		input_data[num++] = entropy[i];
	}

	for(int i=0; i<entropyLen; i++){
		input_data[num++] = addinput02[i];
	}

	for(int i=0; i<num; i++){
		printf("%02x", input_data[i]);
	}printf("\n");

	for(int i=0; i<num; i++){
		ctx->V_secondcall[i] = input_data[i];
	}
	ctx->V_secondcall_length = num;

	fprintf(ctx->file_output, "dfInput = "); //=0x01||V||entropy||addInput
	for(int i=0; i<ctx->V_secondcall_length; i++){
		fprintf(ctx->file_output, "%02x", ctx->V_secondcall[i]);
	}
	fprintf(ctx->file_output, "\n");

	V_DerivedFunction(ctx, rate, capacity, input_data, num, delimitedSuffix, Squeezed, outputByteLen, addinput02);
}

void ResetFunction(struct DRBG_SHA3 *ctx, unsigned int rate, unsigned int capacity, unsigned char delimitedSuffix, unsigned char *output, unsigned long long int outputByteLen, unsigned char *entropy, unsigned char *nonce, unsigned char *perString, unsigned char *addinput01){
	int num = 0;
	BitSequence input_data[1000];
	int entropyLen = 32;
	int nonceLen = 16;
	BitSequence Squeezed[SqueezingOutputLength/8];

	for(int i=0; i<entropyLen; i++){
		input_data[num++] = entropy[i];
	}
	for(int i=0; i<nonceLen; i++){
		input_data[num++] = nonce[i];
	}
	for(int i=0; i<entropyLen; i++){
		input_data[num++] = perString[i];
	}

	for(int i=0; i<num; i++){
		printf("%02x", input_data[i]);
		ctx->dfInput[i] = input_data[i];
	}

	fprintf(ctx->file_output, "dfInput = "); //=entropy||nonce||perString
	for(int i=0; i<num; i++){
		fprintf(ctx->file_output, "%02x", ctx->dfInput[i]);
	}
	fprintf(ctx->file_output, "\n");

	ctx->reseed_counter = 0x00;

	V_DerivedFunction(ctx, rate, capacity, ctx->dfInput, num, delimitedSuffix, Squeezed, outputByteLen, addinput01);
}
