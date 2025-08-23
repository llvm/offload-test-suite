[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() >= 15)) {
        if ((WaveGetLaneIndex() < 4)) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(9));
          }
          uint counter1 = 0;
          while ((counter1 < 2)) {
            counter1 = (counter1 + 1);
            if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
              result = (result + WaveActiveMin(result));
            }
            if ((counter1 == 1)) {
              break;
            }
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
        if ((WaveGetLaneIndex() < 7)) {
          result = (result + WaveActiveSum(result));
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
  if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
    if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
    }
    uint counter2 = 0;
    while ((counter2 < 3)) {
      counter2 = (counter2 + 1);
      if ((WaveGetLaneIndex() == 7)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
      }
      uint counter3 = 0;
      while ((counter3 < 2)) {
        counter3 = (counter3 + 1);
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 6))) {
          result = (result + WaveActiveMin(5));
        }
        if ((counter3 == 1)) {
          break;
        }
      }
      if ((WaveGetLaneIndex() == 15)) {
        result = (result + WaveActiveSum(2));
      }
    }
  } else {
  uint counter4 = 0;
  while ((counter4 < 2)) {
    counter4 = (counter4 + 1);
    if ((WaveGetLaneIndex() >= 11)) {
      if ((WaveGetLaneIndex() >= 10)) {
        result = (result + WaveActiveSum(2));
      }
    }
    if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 8))) {
      result = (result + WaveActiveMin(result));
    }
    if ((counter4 == 1)) {
      break;
    }
  }
  if ((WaveGetLaneIndex() == 8)) {
    result = (result + WaveActiveMax(6));
  }
  }
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
          if (((WaveGetLaneIndex() % 2) == 0)) {
            result = (result + WaveActiveSum(2));
          }
          break;
        }
      case 2: {
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
            if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveSum(4));
            }
            for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
              if (((WaveGetLaneIndex() & 1) == 0)) {
                result = (result + WaveActiveSum(result));
              }
              if ((i5 == 2)) {
                break;
              }
            }
          } else {
          if ((WaveGetLaneIndex() < 6)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          uint counter6 = 0;
          while ((counter6 < 2)) {
            counter6 = (counter6 + 1);
            if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 9))) {
              result = (result + WaveActiveMin(result));
            }
            if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
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
    for (uint i7 = 0; (i7 < 2); i7 = (i7 + 1)) {
      if ((WaveGetLaneIndex() >= 15)) {
        result = (result + WaveActiveMax(result));
      }
      for (uint i8 = 0; (i8 < 2); i8 = (i8 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(result));
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveSum(result));
          }
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMax(result));
          }
        }
        if ((i8 == 1)) {
          continue;
        }
      }
      if ((WaveGetLaneIndex() < 6)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if ((i7 == 1)) {
        break;
      }
    }
    break;
  }
  case 2: {
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      uint counter9 = 0;
      while ((counter9 < 2)) {
        counter9 = (counter9 + 1);
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
        }
        uint counter10 = 0;
        while ((counter10 < 2)) {
          counter10 = (counter10 + 1);
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveMin(4));
          }
          if ((counter10 == 1)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveMax(7));
        }
      }
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
    break;
  }
  }
}
