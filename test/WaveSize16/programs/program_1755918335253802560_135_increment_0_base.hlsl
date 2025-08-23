[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 2)) {
    counter0 = (counter0 + 1);
    if ((WaveGetLaneIndex() < 2)) {
      result = (result + WaveActiveSum(6));
    }
    for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
        result = (result + WaveActiveSum(5));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
        }
      } else {
      if ((WaveGetLaneIndex() == 9)) {
        result = (result + WaveActiveSum(result));
      }
    }
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
    }
    if ((i1 == 1)) {
      break;
    }
  }
  if ((WaveGetLaneIndex() >= 9)) {
    result = (result + WaveActiveSum(WaveGetLaneIndex()));
  }
  }
}
