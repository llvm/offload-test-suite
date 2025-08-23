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
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  case 2: {
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(4));
        }
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMax(7));
          }
          if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15))) {
            if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
          }
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMax(result));
          }
          if ((i1 == 2)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  case 3: {
      if ((WaveGetLaneIndex() == 15)) {
        if ((WaveGetLaneIndex() == 4)) {
          result = (result + WaveActiveMax(result));
        }
        for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(result));
          }
          for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMax(result));
            }
            if ((i3 == 1)) {
              continue;
            }
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(6));
          }
        }
        if ((WaveGetLaneIndex() == 1)) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveSum(result));
      }
      for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
        }
        if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 9))) {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
          }
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 0))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        } else {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveSum(result));
        }
      }
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveMin(result));
      }
    }
    if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 13))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
    }
  }
  break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
  if ((WaveGetLaneIndex() == 13)) {
    result = (result + WaveActiveSum(1));
  } else {
  if ((WaveGetLaneIndex() == 10)) {
    result = (result + WaveActiveMin(2));
  } else {
  if ((WaveGetLaneIndex() == 12)) {
    result = (result + WaveActiveMax(3));
  }
  }
  }
}
