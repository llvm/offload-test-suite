[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() == 10)) {
    if ((WaveGetLaneIndex() == 13)) {
      result = (result + WaveActiveMax(10));
    }
    for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
      if ((WaveGetLaneIndex() < 4)) {
        result = (result + WaveActiveMin(result));
      }
      if ((WaveGetLaneIndex() >= 11)) {
        if ((WaveGetLaneIndex() < 5)) {
          result = (result + WaveActiveMax(result));
        }
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveMin(result));
          }
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveMax(result));
          }
        }
        if ((WaveGetLaneIndex() < 1)) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if ((WaveGetLaneIndex() < 2)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
      }
      uint counter2 = 0;
      while ((counter2 < 2)) {
        counter2 = (counter2 + 1);
        if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveMax(6));
        }
      }
      if ((WaveGetLaneIndex() < 1)) {
        result = (result + WaveActiveSum(result));
      }
    }
    if ((i0 == 1)) {
      continue;
    }
  }
  if ((WaveGetLaneIndex() == 6)) {
    result = (result + WaveActiveMax(2));
  }
  }
}
