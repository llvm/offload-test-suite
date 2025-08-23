[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
    uint counter0 = 0;
    while ((counter0 < 2)) {
      counter0 = (counter0 + 1);
      if ((WaveGetLaneIndex() < 5)) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
        }
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if ((WaveGetLaneIndex() == 5)) {
            result = (result + WaveActiveSum(result));
          }
          if ((i1 == 1)) {
            continue;
          }
        }
      }
      if ((counter0 == 1)) {
        break;
      }
    }
  } else {
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveMin(WaveGetLaneIndex()));
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    result = (result + WaveActiveMax(result));
  }
  }
}
