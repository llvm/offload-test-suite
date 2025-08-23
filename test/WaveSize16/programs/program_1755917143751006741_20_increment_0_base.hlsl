[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() >= 8)) {
            if ((WaveGetLaneIndex() >= 12)) {
              result = (result + WaveActiveMax(result));
            }
            if ((WaveGetLaneIndex() >= 11)) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
            }
          } else {
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveSum(result));
          }
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        break;
      }
    case 1: {
        for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
          if ((WaveGetLaneIndex() >= 10)) {
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
    if ((WaveGetLaneIndex() == 13)) {
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
      if ((WaveGetLaneIndex() == 9)) {
        result = (result + WaveActiveMin(result));
      }
    }
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      uint counter1 = 0;
      while ((counter1 < 3)) {
        counter1 = (counter1 + 1);
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
        }
        uint counter2 = 0;
        while ((counter2 < 2)) {
          counter2 = (counter2 + 1);
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
          if ((counter2 == 1)) {
            break;
          }
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
        }
        if ((counter1 == 2)) {
          break;
        }
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
        switch ((WaveGetLaneIndex() % 4)) {
        case 0: {
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveSum(1));
            }
            break;
          }
        case 1: {
            if ((WaveGetLaneIndex() >= 15)) {
              if ((WaveGetLaneIndex() < 6)) {
                result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
              }
              if ((WaveGetLaneIndex() < 3)) {
                result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
              }
            } else {
            if ((WaveGetLaneIndex() < 3)) {
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
    } else {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMax(7));
    }
    uint counter3 = 0;
    while ((counter3 < 3)) {
      counter3 = (counter3 + 1);
      if ((WaveGetLaneIndex() >= 13)) {
        result = (result + WaveActiveSum(result));
      }
      if ((WaveGetLaneIndex() < 3)) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
    }
  }
  break;
  }
  case 2: {
    if ((WaveGetLaneIndex() == 12)) {
      if ((WaveGetLaneIndex() == 5)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
      }
      if ((WaveGetLaneIndex() == 5)) {
        result = (result + WaveActiveMax(result));
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
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveSum(4));
        }
        uint counter4 = 0;
        while ((counter4 < 3)) {
          counter4 = (counter4 + 1);
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
          }
          if ((counter4 == 2)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  }
}
