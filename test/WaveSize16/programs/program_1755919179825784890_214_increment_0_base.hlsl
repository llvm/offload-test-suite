[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
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
          uint counter0 = 0;
          while ((counter0 < 2)) {
            counter0 = (counter0 + 1);
            if ((WaveGetLaneIndex() == 1)) {
              result = (result + WaveActiveMax(result));
            }
            for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
              if (((WaveGetLaneIndex() & 1) == 1)) {
                result = (result + WaveActiveMax(result));
              }
            }
          }
          break;
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
  if ((WaveGetLaneIndex() == 10)) {
    if ((WaveGetLaneIndex() == 11)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
    }
    if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 5))) {
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 6))) {
        result = (result + WaveActiveMin(result));
      }
      switch ((WaveGetLaneIndex() % 4)) {
      case 0: {
          for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
              result = (result + WaveActiveSum(10));
            }
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
              result = (result + WaveActiveSum(result));
            }
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
            if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
              result = (result + WaveActiveSum(8));
            }
          } else {
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveMin(result));
          }
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
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 0))) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 3))) {
          result = (result + WaveActiveSum(result));
        }
        uint counter3 = 0;
        while ((counter3 < 2)) {
          counter3 = (counter3 + 1);
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
          }
          uint counter4 = 0;
          while ((counter4 < 3)) {
            counter4 = (counter4 + 1);
            if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveMin(result));
            }
            if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 7))) {
              result = (result + WaveActiveSum(result));
            }
          }
        }
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        }
      }
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
      break;
    }
  case 2: {
      for (uint i5 = 0; (i5 < 2); i5 = (i5 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
        }
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveMin(9));
          }
          for (uint i6 = 0; (i6 < 3); i6 = (i6 + 1)) {
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
            if ((i6 == 1)) {
              continue;
            }
          }
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
          }
        }
      }
      break;
    }
  case 3: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() == 14)) {
            if ((WaveGetLaneIndex() == 7)) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
            if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
              if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
                result = (result + WaveActiveSum(WaveGetLaneIndex()));
              }
            }
            if ((WaveGetLaneIndex() == 10)) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(5));
          }
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveMin(9));
            }
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax(result));
          }
        }
        break;
      }
    case 1: {
        for (uint i7 = 0; (i7 < 2); i7 = (i7 + 1)) {
          if ((WaveGetLaneIndex() < 6)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          uint counter8 = 0;
          while ((counter8 < 2)) {
            counter8 = (counter8 + 1);
            if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveMax(7));
            }
            if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9))) {
              result = (result + WaveActiveMax(7));
            }
          }
          if ((WaveGetLaneIndex() < 3)) {
            result = (result + WaveActiveSum(result));
          }
          if ((i7 == 1)) {
            break;
          }
        }
        break;
      }
    case 2: {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
          }
          uint counter9 = 0;
          while ((counter9 < 3)) {
            counter9 = (counter9 + 1);
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveMax(1));
            }
            if ((counter9 == 2)) {
              break;
            }
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax(4));
          }
        }
        break;
      }
    }
    break;
  }
  }
}
