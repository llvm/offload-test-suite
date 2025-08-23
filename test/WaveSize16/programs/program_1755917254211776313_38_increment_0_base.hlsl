[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
    if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
      result = (result + WaveActiveMax(result));
    }
    uint counter0 = 0;
    while ((counter0 < 2)) {
      counter0 = (counter0 + 1);
      if ((WaveGetLaneIndex() < 1)) {
        if ((WaveGetLaneIndex() >= 12)) {
          result = (result + WaveActiveSum(8));
        }
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
          }
          if ((counter1 == 2)) {
            break;
          }
        }
        if ((WaveGetLaneIndex() < 7)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
        }
      } else {
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMax(result));
        }
      }
      if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveSum(result));
      }
    }
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
      result = (result + WaveActiveMin(5));
    }
    if ((counter0 == 1)) {
      break;
    }
  }
  if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
    result = (result + WaveActiveSum(result));
  }
  }
}
