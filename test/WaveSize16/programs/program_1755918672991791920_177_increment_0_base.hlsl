[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 2)) {
    counter0 = (counter0 + 1);
    uint counter1 = 0;
    while ((counter1 < 3)) {
      counter1 = (counter1 + 1);
      if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveSum(5));
      }
      if ((WaveGetLaneIndex() == 2)) {
        if ((WaveGetLaneIndex() == 15)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
        }
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveMin(result));
          }
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
        }
        if ((WaveGetLaneIndex() == 12)) {
          result = (result + WaveActiveMax(9));
        }
      } else {
      if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMin(result));
      }
      uint counter2 = 0;
      while ((counter2 < 2)) {
        counter2 = (counter2 + 1);
        if ((WaveGetLaneIndex() < 1)) {
          result = (result + WaveActiveMin(result));
        }
        if ((WaveGetLaneIndex() >= 13)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveSum(result));
      }
    }
    if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveSum(result));
    }
  }
  if ((counter0 == 1)) {
    break;
  }
  }
  if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if ((WaveGetLaneIndex() >= 12)) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
  }
  }
}
