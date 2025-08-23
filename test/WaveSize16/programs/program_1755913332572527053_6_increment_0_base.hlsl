[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() >= 13)) {
        switch ((WaveGetLaneIndex() % 3)) {
        case 0: {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              if (((WaveGetLaneIndex() & 1) == 0)) {
                result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
              }
              if (((WaveGetLaneIndex() & 1) == 0)) {
                result = (result + WaveActiveMax(result));
              }
            } else {
            if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
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
          if (true) {
            result = (result + WaveActiveSum(3));
          }
          break;
        }
      }
      if ((WaveGetLaneIndex() >= 14)) {
        result = (result + WaveActiveSum(7));
      }
    } else {
    if ((WaveGetLaneIndex() == 11)) {
      result = (result + WaveActiveSum(result));
    }
    for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if ((WaveGetLaneIndex() < 5)) {
          result = (result + WaveActiveMin(6));
        }
      }
      if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
  }
  break;
  }
  case 1: {
    if ((WaveGetLaneIndex() == 5)) {
      if ((WaveGetLaneIndex() == 15)) {
        result = (result + WaveActiveSum(result));
      }
      for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
        if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
        }
        if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 15))) {
          if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMax(result));
          }
        } else {
        if ((WaveGetLaneIndex() == 10)) {
          result = (result + WaveActiveMax(1));
        }
      }
    }
    if ((WaveGetLaneIndex() == 7)) {
      result = (result + WaveActiveMin(3));
    }
  }
  break;
  }
  case 2: {
    if (true) {
      result = (result + WaveActiveSum(3));
    }
  }
  case 3: {
    uint counter3 = 0;
    while ((counter3 < 2)) {
      counter3 = (counter3 + 1);
      if ((WaveGetLaneIndex() >= 13)) {
        result = (result + WaveActiveMax(6));
      }
      if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15))) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
          if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 3))) {
            result = (result + WaveActiveMin(5));
          }
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMax(result));
          }
        }
      } else {
      if ((WaveGetLaneIndex() >= 14)) {
        result = (result + WaveActiveMin(result));
      }
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        }
        if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMin(result));
        }
      }
    }
    if ((WaveGetLaneIndex() < 6)) {
      result = (result + WaveActiveMax(result));
    }
  }
  break;
  }
  default: {
    result = (result + WaveActiveSum(99));
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
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      } else {
      if ((WaveGetLaneIndex() >= 13)) {
        result = (result + WaveActiveSum(result));
      }
    }
    break;
  }
  case 2: {
    if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveMin(result));
      }
      uint counter5 = 0;
      while ((counter5 < 3)) {
        counter5 = (counter5 + 1);
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(result));
        }
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
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      uint counter6 = 0;
      while ((counter6 < 2)) {
        counter6 = (counter6 + 1);
        if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
          uint counter7 = 0;
          while ((counter7 < 2)) {
            counter7 = (counter7 + 1);
            if ((WaveGetLaneIndex() == 9)) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
            if ((WaveGetLaneIndex() == 12)) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
            }
          }
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveMax(result));
          }
        }
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13))) {
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
      uint counter8 = 0;
      while ((counter8 < 3)) {
        counter8 = (counter8 + 1);
        for (uint i9 = 0; (i9 < 3); i9 = (i9 + 1)) {
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
          if ((i9 == 2)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  }
}
