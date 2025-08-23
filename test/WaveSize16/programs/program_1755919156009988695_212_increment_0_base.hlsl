[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        if ((WaveGetLaneIndex() >= 8)) {
          result = (result + WaveActiveSum(4));
        }
        if ((WaveGetLaneIndex() == 10)) {
          if ((WaveGetLaneIndex() == 15)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
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
        if ((i0 == 1)) {
          continue;
        }
      }
      break;
    }
  case 2: {
      if ((WaveGetLaneIndex() == 13)) {
        if ((WaveGetLaneIndex() == 8)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
        }
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          uint counter2 = 0;
          while ((counter2 < 2)) {
            counter2 = (counter2 + 1);
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
          }
          if ((WaveGetLaneIndex() == 5)) {
            result = (result + WaveActiveMax(result));
          }
        }
      } else {
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax(result));
      }
      for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
        if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 0))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
        }
        for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
          if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11))) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
          }
        }
      }
      if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
    }
    break;
  }
  }
  if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 6))) {
    if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 7))) {
      result = (result + WaveActiveMax(result));
    }
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum(1));
        }
        break;
      }
    case 1: {
        for (uint i5 = 0; (i5 < 2); i5 = (i5 + 1)) {
          if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 2))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveSum(2));
          }
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
        if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 6))) {
          result = (result + WaveActiveMax(result));
        }
      }
      break;
    }
  case 2: {
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12))) {
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
      break;
    }
  }
  } else {
  if ((WaveGetLaneIndex() >= 9)) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
  }
  uint counter6 = 0;
  while ((counter6 < 2)) {
    counter6 = (counter6 + 1);
    for (uint i7 = 0; (i7 < 2); i7 = (i7 + 1)) {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveSum(result));
      }
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 4))) {
          result = (result + WaveActiveMin(result));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
    }
  }
  if ((WaveGetLaneIndex() >= 13)) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
  }
  }
  if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
    if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15))) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
    }
    switch ((WaveGetLaneIndex() % 4)) {
    case 0: {
        switch ((WaveGetLaneIndex() % 3)) {
        case 0: {
            if ((WaveGetLaneIndex() == 0)) {
              if ((WaveGetLaneIndex() == 8)) {
                result = (result + WaveActiveMin(result));
              }
              if ((WaveGetLaneIndex() == 0)) {
                result = (result + WaveActiveMax(result));
              }
            }
            break;
          }
        case 1: {
            if ((WaveGetLaneIndex() == 5)) {
              if ((WaveGetLaneIndex() == 12)) {
                result = (result + WaveActiveSum(10));
              }
              if ((WaveGetLaneIndex() == 3)) {
                result = (result + WaveActiveSum(WaveGetLaneIndex()));
              }
            } else {
            if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
          }
          break;
        }
      case 2: {
          if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 12))) {
            if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10))) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
            }
            if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13))) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
            }
          } else {
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 1))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMax(result));
          }
        }
        break;
      }
    }
  }
  case 1: {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMin(result));
      }
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveSum(result));
        }
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveMax(result));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
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
