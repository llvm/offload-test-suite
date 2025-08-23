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
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  case 2: {
      uint counter0 = 0;
      while ((counter0 < 2)) {
        counter0 = (counter0 + 1);
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
            result = (result + WaveActiveMax(result));
          }
        }
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
          result = (result + WaveActiveSum(1));
        }
        if ((counter0 == 1)) {
          break;
        }
      }
      break;
    }
  case 3: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(result));
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
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
  if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
    for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
      if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 1))) {
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 5))) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if ((WaveGetLaneIndex() == 15)) {
        result = (result + WaveActiveMin(result));
      }
      if ((WaveGetLaneIndex() == 5)) {
        result = (result + WaveActiveSum(result));
      }
    }
  }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin(result));
        }
      } else {
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
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 5))) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
      }
    }
    break;
  }
  case 1: {
    if ((WaveGetLaneIndex() == 10)) {
      if ((WaveGetLaneIndex() == 11)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
      }
      for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
        if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 6))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
        }
      }
    }
    break;
  }
  }
}
