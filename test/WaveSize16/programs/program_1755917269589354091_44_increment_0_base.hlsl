[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(result));
        }
      }
      break;
    }
  case 2: {
      uint counter0 = 0;
      while ((counter0 < 2)) {
        counter0 = (counter0 + 1);
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveSum(4));
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
          }
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
            result = (result + WaveActiveMax(8));
          }
        } else {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveSum(result));
      }
    }
    break;
  }
  case 3: {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 4))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
          }
        }
        break;
      }
    case 1: {
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 5))) {
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 2))) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
          }
        }
        break;
      }
    case 2: {
        uint counter2 = 0;
        while ((counter2 < 2)) {
          counter2 = (counter2 + 1);
          if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveMin(7));
          }
          if ((counter2 == 1)) {
            break;
          }
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
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
        for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(result));
          }
          if ((WaveGetLaneIndex() < 3)) {
            if ((WaveGetLaneIndex() < 3)) {
              result = (result + WaveActiveMin(result));
            }
          } else {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 0))) {
            result = (result + WaveActiveMin(result));
          }
        }
      }
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveMin(result));
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
      if ((WaveGetLaneIndex() == 13)) {
        if ((WaveGetLaneIndex() == 14)) {
          result = (result + WaveActiveMax(result));
        }
        uint counter5 = 0;
        while ((counter5 < 3)) {
          counter5 = (counter5 + 1);
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() == 4)) {
            if ((WaveGetLaneIndex() == 1)) {
              result = (result + WaveActiveMax(3));
            }
          }
          if ((WaveGetLaneIndex() == 14)) {
            result = (result + WaveActiveMax(9));
          }
          if ((counter5 == 2)) {
            break;
          }
        }
        if ((WaveGetLaneIndex() == 5)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
        }
      } else {
      for (uint i6 = 0; (i6 < 2); i6 = (i6 + 1)) {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
          result = (result + WaveActiveMax(result));
        }
        uint counter7 = 0;
        while ((counter7 < 2)) {
          counter7 = (counter7 + 1);
          if ((WaveGetLaneIndex() == 11)) {
            result = (result + WaveActiveMin(result));
          }
          if ((counter7 == 1)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if ((i6 == 1)) {
          continue;
        }
        if ((i6 == 1)) {
          break;
        }
      }
      if ((WaveGetLaneIndex() >= 14)) {
        result = (result + WaveActiveSum(result));
      }
    }
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
        for (uint i8 = 0; (i8 < 3); i8 = (i8 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
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
        if ((WaveGetLaneIndex() >= 9)) {
          uint counter9 = 0;
          while ((counter9 < 3)) {
            counter9 = (counter9 + 1);
            if ((WaveGetLaneIndex() < 4)) {
              result = (result + WaveActiveMax(result));
            }
          }
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(2));
          }
        }
        break;
      }
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
