[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() < 6)) {
    for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
      if ((WaveGetLaneIndex() == 11)) {
        if ((WaveGetLaneIndex() == 8)) {
          result = (result + WaveActiveMax(result));
        }
        for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
          if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveSum(result));
          }
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveSum(8));
          }
          if ((i1 == 1)) {
            break;
          }
        }
        if ((WaveGetLaneIndex() == 11)) {
          result = (result + WaveActiveMax(9));
        }
      }
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if ((i0 == 1)) {
        break;
      }
    }
    if ((WaveGetLaneIndex() >= 13)) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
  } else {
  if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
            if ((WaveGetLaneIndex() >= 15)) {
              result = (result + WaveActiveMin(result));
            }
            if ((WaveGetLaneIndex() >= 8)) {
              result = (result + WaveActiveMax(4));
            }
          }
          break;
        }
      case 2: {
          for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveSum(result));
            }
          }
          break;
        }
      }
      break;
    }
  case 2: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 6))) {
            if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveSum(result));
            }
            if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 10))) {
              result = (result + WaveActiveMax(result));
            }
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
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
              result = (result + WaveActiveMin(9));
            }
          } else {
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        break;
      }
    }
    break;
  }
  }
  }
  if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
    for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
      if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveMin(result));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin(10));
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin(result));
        }
      }
      if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMin(result));
      }
    }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      uint counter5 = 0;
      while ((counter5 < 2)) {
        counter5 = (counter5 + 1);
        if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 5))) {
          result = (result + WaveActiveMin(7));
        }
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveSum(result));
          }
          for (uint i6 = 0; (i6 < 2); i6 = (i6 + 1)) {
            if ((WaveGetLaneIndex() == 9)) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
            if ((WaveGetLaneIndex() == 9)) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
            if ((i6 == 1)) {
              continue;
            }
          }
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveSum(result));
          }
        }
      }
      break;
    }
  case 2: {
      switch ((WaveGetLaneIndex() % 2)) {
      case 0: {
          for (uint i7 = 0; (i7 < 3); i7 = (i7 + 1)) {
            if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 15))) {
              if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 13))) {
                result = (result + WaveActiveMin(result));
              }
              if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
                result = (result + WaveActiveMax(5));
              }
            }
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveMin(result));
            }
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() % 2) == 0)) {
            result = (result + WaveActiveSum(2));
          }
          break;
        }
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      break;
    }
  }
}
