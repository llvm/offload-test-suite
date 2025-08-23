[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() >= 13)) {
    result = (result + WaveActiveSum(1));
  } else {
  if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10))) {
    result = (result + WaveActiveMin(2));
  } else {
  if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 1))) {
    result = (result + WaveActiveMax(3));
  } else {
  if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 15))) {
    result = (result + WaveActiveSum(4));
  } else {
  if (((WaveGetLaneIndex() & 1) == 1)) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
  }
  }
  }
  }
  }
}
