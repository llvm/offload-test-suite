[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          switch ((WaveGetLaneIndex() % 3)) {
          case 0: {
              uint counter0 = 0;
              while ((counter0 < 3)) {
                counter0 = (counter0 + 1);
                if ((WaveGetLaneIndex() >= 8)) {
                  result = (result + WaveActiveSum(WaveGetLaneIndex()));
                }
                if ((WaveGetLaneIndex() < 2)) {
                  result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
                }
              }
              break;
            }
          case 1: {
              if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
                if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
                  result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
                }
                if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
                  result = (result + WaveActiveMax(WaveGetLaneIndex()));
                }
              }
              break;
            }
          case 2: {
              if (((WaveGetLaneIndex() & 1) == 1)) {
                if (((WaveGetLaneIndex() & 1) == 1)) {
                  result = (result + WaveActiveMax(4));
                }
              }
              break;
            }
          }
          break;
        }
      case 1: {
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 4))) {
            if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 8))) {
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
            if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 5))) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
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
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMin(result));
        }
        if ((WaveGetLaneIndex() < 1)) {
          if ((WaveGetLaneIndex() >= 10)) {
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
              if (((WaveGetLaneIndex() % 2) == 0)) {
                result = (result + WaveActiveSum(2));
              }
              break;
            }
          case 2: {
              if (true) {
                result = (result + WaveActiveSum(3));
              }
            }
          case 3: {
              if ((WaveGetLaneIndex() < 20)) {
                result = (result + WaveActiveSum(4));
              }
              break;
            }
          }
        } else {
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMax(result));
        }
        if ((WaveGetLaneIndex() < 5)) {
          if ((WaveGetLaneIndex() >= 10)) {
            result = (result + WaveActiveMax(result));
          }
          if ((WaveGetLaneIndex() < 5)) {
            result = (result + WaveActiveMin(result));
          }
        }
      }
      if ((WaveGetLaneIndex() >= 13)) {
        result = (result + WaveActiveSum(result));
      }
    }
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() >= 8)) {
        if ((WaveGetLaneIndex() < 4)) {
          result = (result + WaveActiveMin(result));
        }
        uint counter2 = 0;
        while ((counter2 < 2)) {
          counter2 = (counter2 + 1);
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveMax(5));
            }
            if ((i3 == 1)) {
              break;
            }
          }
        }
        if ((WaveGetLaneIndex() >= 8)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      } else {
      switch ((WaveGetLaneIndex() % 4)) {
      case 0: {
          if ((WaveGetLaneIndex() >= 9)) {
            if ((WaveGetLaneIndex() >= 11)) {
              result = (result + WaveActiveSum(result));
            }
            if ((WaveGetLaneIndex() < 5)) {
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
          if (true) {
            result = (result + WaveActiveSum(3));
          }
          break;
        }
      case 3: {
          if ((WaveGetLaneIndex() == 14)) {
            if ((WaveGetLaneIndex() == 10)) {
              result = (result + WaveActiveMax(3));
            }
            if ((WaveGetLaneIndex() == 4)) {
              result = (result + WaveActiveSum(1));
            }
          }
          break;
        }
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
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
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
          if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
    } else {
    if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
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
  if ((WaveGetLaneIndex() == 1)) {
    if ((WaveGetLaneIndex() == 2)) {
      result = (result + WaveActiveMin(result));
    }
    uint counter4 = 0;
    while ((counter4 < 3)) {
      counter4 = (counter4 + 1);
      for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMax(9));
        }
      }
      if ((counter4 == 2)) {
        break;
      }
    }
  }
}
