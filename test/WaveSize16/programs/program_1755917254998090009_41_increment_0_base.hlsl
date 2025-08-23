[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 8))) {
    if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 6))) {
      result = (result + WaveActiveMin(5));
    }
    uint counter0 = 0;
    while ((counter0 < 2)) {
      counter0 = (counter0 + 1);
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
        }
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
        default: {
            result = (result + WaveActiveSum(99));
            break;
          }
        }
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveSum(result));
        }
      }
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
        result = (result + WaveActiveMax(result));
      }
    }
    if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 4))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
    }
  }
}
