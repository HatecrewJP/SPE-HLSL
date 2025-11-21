


RWBuffer <uint> BufferOut : register(u0);

[numthreads(2,2,1)]
void CSEntry(){
	BufferOut[0]  = 0xA;
	BufferOut[8] = 0xB;
}