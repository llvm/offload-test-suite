[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
          if ((WaveGetLaneIndex() == 10)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
          }
          if ((WaveGetLaneIndex() == 11)) {
            result = (result + WaveActiveSum(result));
          }
        }
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMin(5));
        }
      } else {
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax(result));
      }
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
    }
    break;
  }
  case 1: {
    uint counter2 = 0;
    while ((counter2 < 3)) {
      counter2 = (counter2 + 1);
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMax(result));
      }
      uint counter3 = 0;
      while ((counter3 < 2)) {
        counter3 = (counter3 + 1);
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
        }
      }
    }
    break;
  }
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
}
