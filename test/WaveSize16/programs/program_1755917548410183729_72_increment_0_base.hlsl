[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 4)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
        }
      case 1: {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
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
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMax(result));
            }
          }
          break;
        }
      case 2: {
          uint counter0 = 0;
          while ((counter0 < 2)) {
            counter0 = (counter0 + 1);
            for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
              if ((WaveGetLaneIndex() == 0)) {
                result = (result + WaveActiveMin(10));
              }
              if ((i1 == 1)) {
                continue;
              }
            }
            if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 2))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
          }
          break;
        }
      case 3: {
          if ((WaveGetLaneIndex() < 7)) {
            if ((WaveGetLaneIndex() < 2)) {
              result = (result + WaveActiveMin(result));
            }
            for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
              if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 13))) {
                result = (result + WaveActiveMin(5));
              }
              if ((i2 == 1)) {
                break;
              }
            }
          } else {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(result));
          }
          if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 1))) {
            if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 2))) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
        }
        break;
      }
    }
    break;
  }
  case 1: {
    if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
        result = (result + WaveActiveSum(result));
      }
      for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
        if ((i3 == 1)) {
          continue;
        }
      }
    } else {
    if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      uint counter4 = 0;
      while ((counter4 < 2)) {
        counter4 = (counter4 + 1);
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveSum(5));
        }
      }
    }
  }
  break;
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 11))) {
        for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
          if ((WaveGetLaneIndex() >= 11)) {
            result = (result + WaveActiveSum(result));
          }
        }
        if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveSum(1));
        }
      } else {
      for (uint i6 = 0; (i6 < 2); i6 = (i6 + 1)) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 1))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 2))) {
          result = (result + WaveActiveMax(result));
        }
      }
    }
    break;
  }
  case 1: {
    for (uint i7 = 0; (i7 < 3); i7 = (i7 + 1)) {
      if ((WaveGetLaneIndex() < 3)) {
        result = (result + WaveActiveSum(result));
      }
      for (uint i8 = 0; (i8 < 2); i8 = (i8 + 1)) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum(result));
        }
      }
      if ((WaveGetLaneIndex() >= 12)) {
        result = (result + WaveActiveSum(result));
      }
    }
    break;
  }
  case 2: {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      for (uint i9 = 0; (i9 < 2); i9 = (i9 + 1)) {
        if ((WaveGetLaneIndex() < 6)) {
          result = (result + WaveActiveSum(result));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
      }
    }
    break;
  }
  case 3: {
    if ((WaveGetLaneIndex() < 20)) {
      result = (result + WaveActiveSum(4));
    }
    break;
  }
  }
  uint counter10 = 0;
  while ((counter10 < 2)) {
    counter10 = (counter10 + 1);
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
      if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 14))) {
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMin(result));
        }
        uint counter11 = 0;
        while ((counter11 < 2)) {
          counter11 = (counter11 + 1);
          if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11))) {
            result = (result + WaveActiveMax(result));
          }
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          if ((counter11 == 1)) {
            break;
          }
        }
      }
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
        result = (result + WaveActiveMax(1));
      }
    } else {
    if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
      result = (result + WaveActiveMax(result));
    }
  }
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveSum(8));
  }
  }
}
