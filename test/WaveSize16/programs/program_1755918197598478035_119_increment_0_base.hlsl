[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
        for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
          if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
          }
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveSum(10));
          }
          if ((i0 == 2)) {
            break;
          }
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 7))) {
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
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 9))) {
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 3))) {
          result = (result + WaveActiveMin(result));
        }
        if ((WaveGetLaneIndex() == 7)) {
          if ((WaveGetLaneIndex() == 10)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() == 1)) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
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
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
          if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() == 3)) {
            if ((WaveGetLaneIndex() == 5)) {
              result = (result + WaveActiveMax(result));
            }
          }
        }
        if ((WaveGetLaneIndex() == 13)) {
          result = (result + WaveActiveMin(result));
        }
      }
      break;
    }
  case 2: {
      for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 2))) {
          result = (result + WaveActiveMin(result));
        }
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
          for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
            if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveMin(3));
            }
            if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveMin(result));
            }
            if ((i3 == 1)) {
              continue;
            }
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
