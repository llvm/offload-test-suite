[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 15))) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
  } else {
  if ((WaveGetLaneIndex() < 5)) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
  }
  }
  }
}
