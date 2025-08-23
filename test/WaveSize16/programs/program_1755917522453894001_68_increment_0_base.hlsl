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
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        if ((WaveGetLaneIndex() == 10)) {
          result = (result + WaveActiveMax(5));
        }
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMax(result));
          }
          if ((WaveGetLaneIndex() >= 13)) {
            if ((WaveGetLaneIndex() < 6)) {
              result = (result + WaveActiveMin(result));
            }
          }
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if ((WaveGetLaneIndex() == 1)) {
          result = (result + WaveActiveMin(result));
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
