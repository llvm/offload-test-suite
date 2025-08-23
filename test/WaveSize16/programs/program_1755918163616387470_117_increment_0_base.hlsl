[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 1)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(result));
    }
    for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
      if ((WaveGetLaneIndex() == 10)) {
        result = (result + WaveActiveSum(result));
      }
      if ((WaveGetLaneIndex() == 9)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if ((i0 == 1)) {
        continue;
      }
      if ((i0 == 2)) {
        break;
      }
    }
  } else {
  if (((WaveGetLaneIndex() & 1) == 1)) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
  }
  uint counter1 = 0;
  while ((counter1 < 2)) {
    counter1 = (counter1 + 1);
    if ((WaveGetLaneIndex() >= 15)) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
    if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
        result = (result + WaveActiveMax(8));
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
      }
    } else {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
    if ((WaveGetLaneIndex() >= 11)) {
      if ((WaveGetLaneIndex() < 3)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
      }
    }
  }
  if ((WaveGetLaneIndex() < 6)) {
    result = (result + WaveActiveMax(result));
  }
  }
  }
}
