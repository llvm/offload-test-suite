[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 15))) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
  }
  }
}
