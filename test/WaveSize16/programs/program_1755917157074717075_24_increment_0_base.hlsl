[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 1)) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 15))) {
    result = (result + WaveActiveMin(2));
  } else {
  if ((WaveGetLaneIndex() == 2)) {
    result = (result + WaveActiveMax(3));
  }
  }
  }
}
