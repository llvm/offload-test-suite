[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if ((WaveGetLaneIndex() == 12)) {
    result = (result + WaveActiveMin(2));
  }
  }
}
