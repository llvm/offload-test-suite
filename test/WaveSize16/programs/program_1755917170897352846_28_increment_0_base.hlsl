[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() < 3)) {
    if ((WaveGetLaneIndex() < 8)) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
    }
    for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
      if ((WaveGetLaneIndex() == 1)) {
        result = (result + WaveActiveMax(result));
      }
      if ((WaveGetLaneIndex() == 7)) {
        if ((WaveGetLaneIndex() == 14)) {
          result = (result + WaveActiveMin(result));
        }
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin(result));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(9));
          }
          if ((counter1 == 2)) {
            break;
          }
        }
        if ((WaveGetLaneIndex() == 0)) {
          result = (result + WaveActiveMin(result));
        }
      }
      if ((WaveGetLaneIndex() == 4)) {
        result = (result + WaveActiveMax(result));
      }
    }
    if ((WaveGetLaneIndex() >= 8)) {
      result = (result + WaveActiveMin(result));
    }
  } else {
  if (((WaveGetLaneIndex() & 1) == 1)) {
    result = (result + WaveActiveSum(WaveGetLaneIndex()));
  }
  for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
    if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
      result = (result + WaveActiveSum(result));
    }
    uint counter3 = 0;
    while ((counter3 < 2)) {
      counter3 = (counter3 + 1);
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax(result));
      }
      if ((WaveGetLaneIndex() == 6)) {
        if ((WaveGetLaneIndex() == 7)) {
          result = (result + WaveActiveMin(result));
        }
      }
      if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 0))) {
        result = (result + WaveActiveMax(result));
      }
      if ((counter3 == 1)) {
        break;
      }
    }
    if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveSum(result));
    }
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    result = (result + WaveActiveMax(result));
  }
  }
}
