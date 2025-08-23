[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 1)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMax(result));
    }
    uint counter0 = 0;
    while ((counter0 < 3)) {
      counter0 = (counter0 + 1);
      if ((WaveGetLaneIndex() < 1)) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
      }
      uint counter1 = 0;
      while ((counter1 < 3)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
        }
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
        }
      }
      if ((WaveGetLaneIndex() < 1)) {
        result = (result + WaveActiveMin(result));
      }
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    uint counter2 = 0;
    while ((counter2 < 3)) {
      counter2 = (counter2 + 1);
      if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9))) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
      }
      if ((WaveGetLaneIndex() == 0)) {
        if ((WaveGetLaneIndex() == 12)) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
          if ((WaveGetLaneIndex() == 3)) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() == 7)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
          }
          if ((i3 == 1)) {
            continue;
          }
          if ((i3 == 1)) {
            break;
          }
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveSum(result));
      }
      uint counter4 = 0;
      while ((counter4 < 2)) {
        counter4 = (counter4 + 1);
        if ((WaveGetLaneIndex() == 6)) {
          result = (result + WaveActiveMin(result));
        }
        if ((WaveGetLaneIndex() == 12)) {
          result = (result + WaveActiveSum(result));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveSum(result));
      }
    }
    if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveMax(10));
    }
  }
  }
}
