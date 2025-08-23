[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 5))) {
    for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
      if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 3))) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 1))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
        }
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 0))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 9))) {
            result = (result + WaveActiveMin(7));
          }
          if ((counter1 == 2)) {
            break;
          }
        }
      } else {
      if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveMax(result));
      }
      uint counter2 = 0;
      while ((counter2 < 2)) {
        counter2 = (counter2 + 1);
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
          result = (result + WaveActiveSum(2));
        }
      }
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMax(result));
    }
  }
  if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 2))) {
    result = (result + WaveActiveMax(result));
  }
  } else {
  uint counter3 = 0;
  while ((counter3 < 2)) {
    counter3 = (counter3 + 1);
    for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
      if ((WaveGetLaneIndex() < 8)) {
        if ((WaveGetLaneIndex() < 2)) {
          result = (result + WaveActiveMax(result));
        }
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
      if ((WaveGetLaneIndex() >= 10)) {
        result = (result + WaveActiveMax(result));
      }
      if ((i4 == 1)) {
        continue;
      }
    }
    if ((WaveGetLaneIndex() == 14)) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
    }
  }
  if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 14))) {
    result = (result + WaveActiveMin(2));
  }
  }
}
