[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 1)) {
    if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11))) {
      switch ((WaveGetLaneIndex() % 2)) {
      case 0: {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMin(8));
            }
          }
          break;
        }
      case 1: {
          for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
            if ((WaveGetLaneIndex() < 3)) {
              result = (result + WaveActiveMin(result));
            }
            if ((WaveGetLaneIndex() >= 11)) {
              result = (result + WaveActiveMin(result));
            }
          }
          break;
        }
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMin(result));
    }
  } else {
  if (((WaveGetLaneIndex() & 1) == 1)) {
    result = (result + WaveActiveMax(WaveGetLaneIndex()));
  }
  if ((WaveGetLaneIndex() == 8)) {
    if ((WaveGetLaneIndex() == 11)) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
    }
    for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
      if ((WaveGetLaneIndex() >= 11)) {
        result = (result + WaveActiveSum(result));
      }
      for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if ((WaveGetLaneIndex() >= 10)) {
        result = (result + WaveActiveSum(4));
      }
      if ((i1 == 1)) {
        continue;
      }
    }
    if ((WaveGetLaneIndex() == 13)) {
      result = (result + WaveActiveMax(result));
    }
  }
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveMax(3));
  }
  }
  for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
    if ((WaveGetLaneIndex() >= 12)) {
      result = (result + WaveActiveMax(result));
    }
    for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
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
          if (true) {
            result = (result + WaveActiveSum(3));
          }
          break;
        }
      case 3: {
          if ((WaveGetLaneIndex() < 20)) {
            result = (result + WaveActiveSum(4));
          }
          break;
        }
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveSum(result));
      }
      if ((i4 == 1)) {
        break;
      }
    }
    if ((i3 == 2)) {
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
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
  }
}
