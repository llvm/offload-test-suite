[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveSum(result));
        }
        for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveSum(1));
          }
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin(7));
          }
        }
      }
    }
  case 2: {
      if (true) {
        result = (result + WaveActiveSum(3));
      }
    }
  case 3: {
      if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 15))) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum(result));
        }
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 4))) {
          if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 1))) {
            result = (result + WaveActiveSum(result));
          }
        }
      } else {
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveSum(result));
      }
      uint counter1 = 0;
      while ((counter1 < 3)) {
        counter1 = (counter1 + 1);
        if ((WaveGetLaneIndex() < 5)) {
          result = (result + WaveActiveMax(result));
        }
        if ((WaveGetLaneIndex() < 1)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
        }
      }
    }
    break;
  }
  }
  uint counter2 = 0;
  while ((counter2 < 2)) {
    counter2 = (counter2 + 1);
    if ((WaveGetLaneIndex() >= 14)) {
      if ((WaveGetLaneIndex() >= 14)) {
        result = (result + WaveActiveSum(result));
      }
      uint counter3 = 0;
      while ((counter3 < 2)) {
        counter3 = (counter3 + 1);
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
      }
      if ((WaveGetLaneIndex() >= 15)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
      }
    } else {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveSum(result));
      }
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() % 2) == 0)) {
            result = (result + WaveActiveSum(2));
          }
          break;
        }
      case 2: {
          if (true) {
            result = (result + WaveActiveSum(3));
          }
          break;
        }
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMax(5));
      }
    }
    if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 0))) {
      result = (result + WaveActiveSum(result));
    }
  }
  if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11))) {
    result = (result + WaveActiveSum(1));
  }
  }
  for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
    if ((WaveGetLaneIndex() < 4)) {
      result = (result + WaveActiveMax(10));
    }
    if ((WaveGetLaneIndex() >= 12)) {
      result = (result + WaveActiveMin(result));
    }
  }
}
