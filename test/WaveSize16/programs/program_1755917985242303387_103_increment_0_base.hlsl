[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if ((WaveGetLaneIndex() >= 11)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if ((i0 == 2)) {
          break;
        }
      }
      break;
    }
  case 1: {
      for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        uint counter3 = 0;
        while ((counter3 < 2)) {
          counter3 = (counter3 + 1);
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(result));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(9));
        }
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
  if ((WaveGetLaneIndex() < 1)) {
    switch ((WaveGetLaneIndex() % 4)) {
    case 0: {
        if ((WaveGetLaneIndex() == 6)) {
          if ((WaveGetLaneIndex() == 2)) {
            result = (result + WaveActiveMin(result));
          }
        }
      }
    case 1: {
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11))) {
          if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
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
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      uint counter4 = 0;
      while ((counter4 < 3)) {
        counter4 = (counter4 + 1);
        if ((WaveGetLaneIndex() == 9)) {
          if ((WaveGetLaneIndex() == 15)) {
            result = (result + WaveActiveMax(4));
          }
          if ((WaveGetLaneIndex() == 2)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
          }
        }
        if ((WaveGetLaneIndex() < 3)) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  case 2: {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        for (uint i5 = 0; (i5 < 2); i5 = (i5 + 1)) {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
          }
          if ((i5 == 1)) {
            continue;
          }
        }
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if ((WaveGetLaneIndex() == 7)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
      }
      for (uint i6 = 0; (i6 < 2); i6 = (i6 + 1)) {
        if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveMin(result));
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
        }
        if ((i6 == 1)) {
          break;
        }
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
