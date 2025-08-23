[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 2)) {
    counter0 = (counter0 + 1);
    if ((WaveGetLaneIndex() == 6)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 3))) {
      if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 8))) {
        result = (result + WaveActiveMin(2));
      }
      if ((WaveGetLaneIndex() >= 14)) {
        if ((WaveGetLaneIndex() >= 11)) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if ((WaveGetLaneIndex() == 6)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() == 13)) {
            result = (result + WaveActiveSum(result));
          }
        }
      }
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 3))) {
        result = (result + WaveActiveMin(2));
      }
    } else {
    uint counter2 = 0;
    while ((counter2 < 3)) {
      counter2 = (counter2 + 1);
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
        }
      }
      if ((counter2 == 2)) {
        break;
      }
    }
  }
  if ((WaveGetLaneIndex() == 13)) {
    result = (result + WaveActiveMin(result));
  }
  if ((counter0 == 1)) {
    break;
  }
  }
  if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 8))) {
    if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 5))) {
      result = (result + WaveActiveMin(result));
    }
    for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 2))) {
        result = (result + WaveActiveMin(2));
      }
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 11))) {
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMin(result));
        }
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 3))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
        }
      }
    }
    if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 6))) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
  }
  if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 14))) {
    if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveSum(4));
    }
    switch ((WaveGetLaneIndex() % 4)) {
    case 0: {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum(1));
        }
        break;
      }
    case 1: {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          uint counter4 = 0;
          while ((counter4 < 2)) {
            counter4 = (counter4 + 1);
            if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 3))) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
          }
        } else {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        uint counter5 = 0;
        while ((counter5 < 2)) {
          counter5 = (counter5 + 1);
          if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 11))) {
            result = (result + WaveActiveSum(result));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
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
  case 3: {
      if ((WaveGetLaneIndex() < 20)) {
        result = (result + WaveActiveSum(4));
      }
      break;
    }
  }
  } else {
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      if ((WaveGetLaneIndex() == 9)) {
        if ((WaveGetLaneIndex() == 9)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
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
        }
        if ((WaveGetLaneIndex() == 5)) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if ((WaveGetLaneIndex() == 0)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
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
      }
      if ((WaveGetLaneIndex() == 10)) {
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
  if ((WaveGetLaneIndex() < 4)) {
    result = (result + WaveActiveSum(result));
  }
  }
}
