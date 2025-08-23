[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveSum(1));
  } else {
  if ((WaveGetLaneIndex() == 13)) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
  } else {
  if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
    result = (result + WaveActiveMax(3));
  } else {
  if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 0))) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
  }
  }
  }
  }
}
