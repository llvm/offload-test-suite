[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() >= 14)) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if ((WaveGetLaneIndex() >= 14)) {
    result = (result + WaveActiveMin(2));
  } else {
  if ((WaveGetLaneIndex() == 8)) {
    result = (result + WaveActiveMax(3));
  } else {
  if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveSum(4));
  } else {
  if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 10))) {
    result = (result + WaveActiveMin(5));
  }
  }
  }
  }
  }
}
