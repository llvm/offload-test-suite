[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() < 8)) {
    if ((WaveGetLaneIndex() >= 14)) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
    }
    uint counter0 = 0;
    while ((counter0 < 3)) {
      counter0 = (counter0 + 1);
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
        }
      }
    }
    if ((WaveGetLaneIndex() >= 13)) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
    }
  }
  if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
    if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
    }
    for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
      if ((WaveGetLaneIndex() == 9)) {
        result = (result + WaveActiveMin(result));
      }
      if ((WaveGetLaneIndex() < 2)) {
        if ((WaveGetLaneIndex() >= 9)) {
          result = (result + WaveActiveMax(6));
        }
        if ((WaveGetLaneIndex() < 1)) {
          result = (result + WaveActiveMax(7));
        }
      }
      if ((WaveGetLaneIndex() == 7)) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
    }
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
    }
  }
}
