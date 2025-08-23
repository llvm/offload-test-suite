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
      if ((WaveGetLaneIndex() >= 10)) {
        if ((WaveGetLaneIndex() >= 9)) {
          result = (result + WaveActiveSum(result));
        }
        if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9))) {
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
            if ((WaveGetLaneIndex() >= 12)) {
              result = (result + WaveActiveMax(result));
            }
            if ((WaveGetLaneIndex() >= 14)) {
              result = (result + WaveActiveMax(result));
            }
          }
        }
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
        }
      } else {
      if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 12))) {
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
          if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 6))) {
            result = (result + WaveActiveMin(7));
          }
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
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
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10))) {
        switch ((WaveGetLaneIndex() % 2)) {
        case 0: {
            if ((WaveGetLaneIndex() == 7)) {
              if ((WaveGetLaneIndex() == 10)) {
                result = (result + WaveActiveMax(result));
              }
              if ((WaveGetLaneIndex() == 7)) {
                result = (result + WaveActiveMin(result));
              }
            }
            break;
          }
        case 1: {
            if (((WaveGetLaneIndex() & 1) == 0)) {
              if (((WaveGetLaneIndex() & 1) == 0)) {
                result = (result + WaveActiveMax(result));
              }
            }
            break;
          }
        }
        if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMax(8));
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin(result));
      }
    }
    break;
  }
  case 1: {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveSum(result));
        }
        uint counter2 = 0;
        while ((counter2 < 3)) {
          counter2 = (counter2 + 1);
          if ((WaveGetLaneIndex() >= 11)) {
            result = (result + WaveActiveMax(result));
          }
          if ((counter2 == 2)) {
            break;
          }
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin(result));
      }
    } else {
    if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
      result = (result + WaveActiveMin(result));
    }
    for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
        result = (result + WaveActiveSum(result));
      }
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
    }
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
    }
  }
  if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11))) {
    result = (result + WaveActiveSum(result));
  }
  }
  break;
  }
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
        result = (result + WaveActiveMax(result));
      }
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 7))) {
        if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMax(result));
        }
      }
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(result));
    }
  }
}
