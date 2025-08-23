[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() >= 15)) {
    result = (result + WaveActiveSum(1));
  } else {
  if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 4))) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
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
          switch ((WaveGetLaneIndex() % 3)) {
          case 0: {
              if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
                if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
                  result = (result + WaveActiveMax(result));
                }
                if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
                  result = (result + WaveActiveMax(WaveGetLaneIndex()));
                }
              } else {
              if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
                result = (result + WaveActiveMax(9));
              }
            }
            break;
          }
        case 1: {
            uint counter0 = 0;
            while ((counter0 < 3)) {
              counter0 = (counter0 + 1);
              if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
                result = (result + WaveActiveSum(4));
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
    }
    break;
  }
  case 1: {
    if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveSum(result));
        }
        if ((WaveGetLaneIndex() >= 11)) {
          if ((WaveGetLaneIndex() < 2)) {
            result = (result + WaveActiveMax(1));
          }
          if ((WaveGetLaneIndex() >= 13)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
          }
        } else {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
        }
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveMin(result));
        }
      }
    }
  } else {
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveMax(2));
  }
  for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
    if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveSum(result));
    }
    uint counter3 = 0;
    while ((counter3 < 2)) {
      counter3 = (counter3 + 1);
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveMin(result));
    }
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    result = (result + WaveActiveSum(result));
  }
  }
  break;
  }
  case 2: {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        uint counter4 = 0;
        while ((counter4 < 2)) {
          counter4 = (counter4 + 1);
          if (((WaveGetLaneIndex() & 1) == 1)) {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
            }
          } else {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMax(3));
          }
        }
      }
      break;
    }
  case 1: {
      for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
        }
      }
      break;
    }
  case 2: {
      uint counter6 = 0;
      while ((counter6 < 2)) {
        counter6 = (counter6 + 1);
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 3))) {
          result = (result + WaveActiveMin(result));
        }
      }
      break;
    }
  }
  break;
  }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      for (uint i7 = 0; (i7 < 2); i7 = (i7 + 1)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMax(result));
        }
        if ((WaveGetLaneIndex() < 4)) {
          result = (result + WaveActiveMax(9));
        }
        if ((i7 == 1)) {
          continue;
        }
        if ((i7 == 1)) {
          break;
        }
      }
      break;
    }
  case 1: {
      if ((WaveGetLaneIndex() == 2)) {
        for (uint i8 = 0; (i8 < 2); i8 = (i8 + 1)) {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
          }
          for (uint i9 = 0; (i9 < 2); i9 = (i9 + 1)) {
            if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 13))) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
            }
          }
          if ((i8 == 1)) {
            continue;
          }
        }
        if ((WaveGetLaneIndex() == 5)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
      break;
    }
  }
}
