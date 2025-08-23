[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveSum(8));
        }
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
          }
        }
        if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveMin(result));
        }
        if ((i0 == 2)) {
          break;
        }
      }
    }
  case 1: {
      for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(9));
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(1));
          }
        }
        if ((i2 == 2)) {
          break;
        }
      }
    }
  case 2: {
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 9))) {
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 2))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        if ((WaveGetLaneIndex() < 5)) {
          if ((WaveGetLaneIndex() < 1)) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() < 5)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 5))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
        }
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
  for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
    }
    for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
      if ((WaveGetLaneIndex() == 4)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveMin(result));
        }
        uint counter5 = 0;
        while ((counter5 < 2)) {
          counter5 = (counter5 + 1);
          if ((WaveGetLaneIndex() == 15)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
          }
        }
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
        }
      }
      if ((WaveGetLaneIndex() == 15)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
      }
    }
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
    }
    if ((i3 == 1)) {
      break;
    }
  }
}
