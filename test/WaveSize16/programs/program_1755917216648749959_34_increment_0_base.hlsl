[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
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
      }
    }
  case 2: {
      uint counter0 = 0;
      while ((counter0 < 2)) {
        counter0 = (counter0 + 1);
        if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
          if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() < 1)) {
            if ((WaveGetLaneIndex() < 4)) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
            if ((WaveGetLaneIndex() < 7)) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
          } else {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
          }
        }
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
        }
      }
      if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
      }
    }
    break;
  }
  case 3: {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        switch ((WaveGetLaneIndex() % 3)) {
        case 0: {
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveSum(1));
            }
            break;
          }
        case 1: {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              if (((WaveGetLaneIndex() & 1) == 0)) {
                result = (result + WaveActiveSum(result));
              }
              if (((WaveGetLaneIndex() & 1) == 1)) {
                result = (result + WaveActiveMax(WaveGetLaneIndex()));
              }
            } else {
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
            }
          }
          break;
        }
      case 2: {
          uint counter2 = 0;
          while ((counter2 < 3)) {
            counter2 = (counter2 + 1);
            if ((WaveGetLaneIndex() == 11)) {
              result = (result + WaveActiveMax(result));
            }
          }
          break;
        }
      }
      break;
    }
  case 1: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
            }
            if ((i3 == 1)) {
              continue;
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
      break;
    }
  case 2: {
      if (true) {
        result = (result + WaveActiveSum(3));
      }
      break;
    }
  }
  break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
  if ((WaveGetLaneIndex() < 8)) {
    if ((WaveGetLaneIndex() >= 12)) {
      result = (result + WaveActiveMin(8));
    }
    switch ((WaveGetLaneIndex() % 2)) {
    case 0: {
        for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
          if ((WaveGetLaneIndex() == 10)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
          }
          uint counter5 = 0;
          while ((counter5 < 3)) {
            counter5 = (counter5 + 1);
            if ((WaveGetLaneIndex() < 5)) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
            if ((counter5 == 2)) {
              break;
            }
          }
          if ((i4 == 1)) {
            break;
          }
        }
        break;
      }
    case 1: {
        uint counter6 = 0;
        while ((counter6 < 2)) {
          counter6 = (counter6 + 1);
          if ((WaveGetLaneIndex() == 6)) {
            result = (result + WaveActiveMin(result));
          }
          if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 10))) {
            if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveMax(result));
            }
            if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 13))) {
              result = (result + WaveActiveMax(result));
            }
          } else {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin(result));
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
          }
        }
        if ((counter6 == 1)) {
          break;
        }
      }
      break;
    }
  }
  if ((WaveGetLaneIndex() >= 8)) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
  }
  }
}
