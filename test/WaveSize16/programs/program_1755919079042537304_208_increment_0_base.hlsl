[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      uint counter0 = 0;
      while ((counter0 < 2)) {
        counter0 = (counter0 + 1);
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 10))) {
            result = (result + WaveActiveMin(1));
          }
          if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 0))) {
            result = (result + WaveActiveMax(result));
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
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 1))) {
          result = (result + WaveActiveMax(result));
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveSum(result));
          }
        } else {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
        }
      }
      if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveMax(1));
      }
      if ((i2 == 1)) {
        continue;
      }
    }
    break;
  }
  case 1: {
    for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
      if ((WaveGetLaneIndex() == 14)) {
        result = (result + WaveActiveMin(4));
      }
      if ((i3 == 2)) {
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
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        uint counter4 = 0;
        while ((counter4 < 3)) {
          counter4 = (counter4 + 1);
          if ((WaveGetLaneIndex() == 6)) {
            result = (result + WaveActiveMax(result));
          }
        }
      } else {
      if ((WaveGetLaneIndex() == 0)) {
        result = (result + WaveActiveMax(result));
      }
      if ((WaveGetLaneIndex() == 9)) {
        if ((WaveGetLaneIndex() == 4)) {
          result = (result + WaveActiveMin(result));
        }
      }
    }
  }
  case 1: {
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
  }
  case 2: {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10))) {
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMax(result));
          }
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
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
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 1))) {
          if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 5))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
        } else {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMin(result));
        }
      }
      break;
    }
  }
  break;
  }
  }
}
