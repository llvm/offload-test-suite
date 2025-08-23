[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
    }
    uint counter1 = 0;
    while ((counter1 < 3)) {
      counter1 = (counter1 + 1);
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveSum(1));
      }
      if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
    }
  }
  if ((i0 == 2)) {
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
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
          if (((WaveGetLaneIndex() % 2) == 0)) {
            result = (result + WaveActiveSum(2));
          }
          break;
        }
      case 2: {
          if ((WaveGetLaneIndex() >= 13)) {
            if ((WaveGetLaneIndex() >= 8)) {
              result = (result + WaveActiveSum(result));
            }
          }
          break;
        }
      }
    }
  case 2: {
      if (true) {
        result = (result + WaveActiveSum(3));
      }
    }
  case 3: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
          } else {
          if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveSum(6));
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
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 4))) {
          if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 7))) {
            result = (result + WaveActiveMin(result));
          }
        } else {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveMax(result));
        }
        if ((WaveGetLaneIndex() < 2)) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  }
  break;
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
        uint counter3 = 0;
        while ((counter3 < 3)) {
          counter3 = (counter3 + 1);
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
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
          default: {
              result = (result + WaveActiveSum(99));
              break;
            }
          }
        }
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
          result = (result + WaveActiveMax(4));
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
      for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
        uint counter5 = 0;
        while ((counter5 < 3)) {
          counter5 = (counter5 + 1);
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin(result));
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
          default: {
              result = (result + WaveActiveSum(99));
              break;
            }
          }
        }
      }
      break;
    }
  }
}
