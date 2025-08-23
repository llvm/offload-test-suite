[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
            if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
            uint counter0 = 0;
            while ((counter0 < 2)) {
              counter0 = (counter0 + 1);
              if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
                result = (result + WaveActiveMax(result));
              }
            }
          } else {
          if ((WaveGetLaneIndex() == 12)) {
            result = (result + WaveActiveMax(result));
          }
          uint counter1 = 0;
          while ((counter1 < 2)) {
            counter1 = (counter1 + 1);
            if ((WaveGetLaneIndex() == 8)) {
              result = (result + WaveActiveSum(6));
            }
          }
          if ((WaveGetLaneIndex() == 13)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
          }
        }
        break;
      }
    case 1: {
        if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 3))) {
          uint counter2 = 0;
          while ((counter2 < 2)) {
            counter2 = (counter2 + 1);
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveSum(result));
            }
          }
        } else {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMax(8));
        }
        uint counter3 = 0;
        while ((counter3 < 3)) {
          counter3 = (counter3 + 1);
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveSum(1));
          }
        }
      }
      break;
    }
  case 2: {
      for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
        if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 5))) {
          result = (result + WaveActiveMin(3));
        }
        for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
        if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveMax(10));
        }
        if ((i4 == 2)) {
          break;
        }
      }
      break;
    }
  }
  }
  case 1: {
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
        if ((WaveGetLaneIndex() == 3)) {
          uint counter6 = 0;
          while ((counter6 < 3)) {
            counter6 = (counter6 + 1);
            if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveSum(result));
            }
            if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10))) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
            }
          }
          if ((WaveGetLaneIndex() == 2)) {
            result = (result + WaveActiveSum(2));
          }
        } else {
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 12))) {
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
  }
  case 2: {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          switch ((WaveGetLaneIndex() % 2)) {
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
          default: {
              result = (result + WaveActiveSum(99));
              break;
            }
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(result));
          }
        } else {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
        }
        if ((WaveGetLaneIndex() < 7)) {
          if ((WaveGetLaneIndex() < 1)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
          }
          if ((WaveGetLaneIndex() >= 10)) {
            result = (result + WaveActiveMin(result));
          }
        }
      }
      break;
    }
  case 1: {
      uint counter7 = 0;
      while ((counter7 < 2)) {
        counter7 = (counter7 + 1);
        if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMax(result));
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
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 6)) {
        if ((WaveGetLaneIndex() >= 10)) {
          result = (result + WaveActiveMax(result));
        }
        uint counter8 = 0;
        while ((counter8 < 2)) {
          counter8 = (counter8 + 1);
          uint counter9 = 0;
          while ((counter9 < 3)) {
            counter9 = (counter9 + 1);
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMax(result));
            }
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveSum(result));
            }
          }
        }
        if ((WaveGetLaneIndex() >= 11)) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
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
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
            if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11))) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
            }
            if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveMin(9));
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
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
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
    for (uint i10 = 0; (i10 < 3); i10 = (i10 + 1)) {
      if ((WaveGetLaneIndex() == 13)) {
        result = (result + WaveActiveMin(result));
      }
      uint counter11 = 0;
      while ((counter11 < 3)) {
        counter11 = (counter11 + 1);
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMax(result));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(result));
          }
        }
        if ((counter11 == 2)) {
          break;
        }
      }
      if ((WaveGetLaneIndex() == 2)) {
        result = (result + WaveActiveSum(result));
      }
    }
    break;
  }
  case 3: {
    if ((WaveGetLaneIndex() < 7)) {
      if ((WaveGetLaneIndex() >= 9)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
      }
      uint counter12 = 0;
      while ((counter12 < 3)) {
        counter12 = (counter12 + 1);
        if ((WaveGetLaneIndex() >= 11)) {
          result = (result + WaveActiveSum(result));
        }
        if ((WaveGetLaneIndex() < 5)) {
          if ((WaveGetLaneIndex() >= 8)) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((WaveGetLaneIndex() < 2)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
        }
      }
    } else {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
    }
    for (uint i13 = 0; (i13 < 3); i13 = (i13 + 1)) {
      uint counter14 = 0;
      while ((counter14 < 2)) {
        counter14 = (counter14 + 1);
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if ((i13 == 1)) {
        continue;
      }
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
      for (uint i15 = 0; (i15 < 3); i15 = (i15 + 1)) {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveSum(result));
        }
        if ((WaveGetLaneIndex() == 0)) {
          if ((WaveGetLaneIndex() == 7)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveSum(10));
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
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
        for (uint i16 = 0; (i16 < 2); i16 = (i16 + 1)) {
          if ((WaveGetLaneIndex() < 1)) {
            result = (result + WaveActiveMin(result));
          }
          if ((WaveGetLaneIndex() >= 14)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
          }
        }
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      for (uint i17 = 0; (i17 < 3); i17 = (i17 + 1)) {
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin(result));
      }
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
}
