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
      uint counter0 = 0;
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        if ((WaveGetLaneIndex() >= 8)) {
          if ((WaveGetLaneIndex() >= 9)) {
            result = (result + WaveActiveSum(result));
          }
        }
        if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 7))) {
          result = (result + WaveActiveSum(result));
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
    }
  case 3: {
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        uint counter2 = 0;
        while ((counter2 < 2)) {
          counter2 = (counter2 + 1);
          if ((WaveGetLaneIndex() == 6)) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveMax(6));
          }
        }
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
        }
        if ((counter1 == 1)) {
          break;
        }
      }
      break;
    }
  }
}
