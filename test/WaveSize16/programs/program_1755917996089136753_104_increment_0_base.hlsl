[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 9))) {
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveSum(2));
        }
        uint counter0 = 0;
        while ((counter0 < 2)) {
          counter0 = (counter0 + 1);
          if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
          }
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveSum(result));
          }
        }
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 9))) {
        result = (result + WaveActiveMin(result));
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(1));
        }
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
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum(1));
        }
        break;
      }
    case 1: {
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 15))) {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
            result = (result + WaveActiveMin(result));
          }
        } else {
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMin(result));
        }
        if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
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
    for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
      if ((WaveGetLaneIndex() < 3)) {
        if ((WaveGetLaneIndex() >= 12)) {
          result = (result + WaveActiveMax(10));
        }
      } else {
      if ((WaveGetLaneIndex() == 12)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
    if ((WaveGetLaneIndex() == 5)) {
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
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMax(8));
        }
        if ((WaveGetLaneIndex() < 3)) {
          if ((WaveGetLaneIndex() < 7)) {
            result = (result + WaveActiveMax(8));
          }
        } else {
        if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
      if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveMax(5));
      }
    }
    break;
  }
  case 1: {
    uint counter3 = 0;
    while ((counter3 < 2)) {
      counter3 = (counter3 + 1);
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 9))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
      uint counter4 = 0;
      while ((counter4 < 3)) {
        counter4 = (counter4 + 1);
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
          result = (result + WaveActiveMin(result));
        }
      }
      if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 0))) {
        result = (result + WaveActiveMin(result));
      }
      if ((counter3 == 1)) {
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
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() == 15)) {
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
          }
          for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
              result = (result + WaveActiveMin(8));
            }
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveSum(result));
            }
          }
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveSum(result));
          }
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
    }
    break;
  }
  case 1: {
    uint counter6 = 0;
    while ((counter6 < 3)) {
      counter6 = (counter6 + 1);
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 2))) {
        result = (result + WaveActiveMin(result));
      }
      uint counter7 = 0;
      while ((counter7 < 2)) {
        counter7 = (counter7 + 1);
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveSum(9));
        }
        if ((WaveGetLaneIndex() >= 8)) {
          if ((WaveGetLaneIndex() >= 10)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        } else {
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveSum(4));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveSum(result));
      }
    }
    if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
    }
  }
  break;
  }
  case 2: {
    if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 9))) {
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 6))) {
        result = (result + WaveActiveMin(4));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
        }
        uint counter8 = 0;
        while ((counter8 < 2)) {
          counter8 = (counter8 + 1);
          if ((WaveGetLaneIndex() >= 9)) {
            result = (result + WaveActiveSum(result));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
      }
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 9))) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
      }
    } else {
    if ((WaveGetLaneIndex() == 7)) {
      result = (result + WaveActiveSum(result));
    }
    uint counter9 = 0;
    while ((counter9 < 3)) {
      counter9 = (counter9 + 1);
      if ((WaveGetLaneIndex() >= 15)) {
        result = (result + WaveActiveMax(10));
      }
      if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 13))) {
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
        }
      }
      if ((WaveGetLaneIndex() < 5)) {
        result = (result + WaveActiveSum(3));
      }
    }
    if ((WaveGetLaneIndex() == 14)) {
      result = (result + WaveActiveMin(result));
    }
  }
  break;
  }
  case 3: {
    for (uint i10 = 0; (i10 < 3); i10 = (i10 + 1)) {
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
        result = (result + WaveActiveMin(result));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(result));
        }
        uint counter11 = 0;
        while ((counter11 < 3)) {
          counter11 = (counter11 + 1);
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(7));
          }
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if ((WaveGetLaneIndex() == 10)) {
        result = (result + WaveActiveSum(10));
      }
      switch ((WaveGetLaneIndex() % 2)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
        }
      case 1: {
          if (((WaveGetLaneIndex() % 2) == 0)) {
            result = (result + WaveActiveSum(2));
          }
          break;
        }
      }
    }
    if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 11))) {
      result = (result + WaveActiveSum(result));
    }
  }
  break;
  }
  }
}
