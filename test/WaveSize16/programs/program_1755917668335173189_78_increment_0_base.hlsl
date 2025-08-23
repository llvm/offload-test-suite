[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
    uint counter1 = 0;
    while ((counter1 < 2)) {
      counter1 = (counter1 + 1);
      if ((WaveGetLaneIndex() == 9)) {
        if ((WaveGetLaneIndex() == 12)) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 7))) {
        result = (result + WaveActiveMin(result));
      }
      if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
  }
  }
  if ((WaveGetLaneIndex() >= 15)) {
    result = (result + WaveActiveSum(1));
  } else {
  if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 6))) {
    result = (result + WaveActiveMin(2));
  } else {
  if ((WaveGetLaneIndex() == 14)) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
  } else {
  if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14))) {
    result = (result + WaveActiveSum(4));
  } else {
  if ((WaveGetLaneIndex() == 9)) {
    result = (result + WaveActiveMin(5));
  }
  }
  }
  }
  }
}
