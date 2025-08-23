[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
    }
  case 1: {
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 0))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(2));
          }
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 0))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
        }
        if ((i0 == 2)) {
          break;
        }
      }
      break;
    }
  case 2: {
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
      break;
    }
  case 3: {
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if ((WaveGetLaneIndex() < 1)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
        }
        uint counter2 = 0;
        while ((counter2 < 3)) {
          counter2 = (counter2 + 1);
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(6));
          }
        }
        if ((counter1 == 1)) {
          break;
        }
      }
      break;
    }
  }
}
