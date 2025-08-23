[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 3)) {
    counter0 = (counter0 + 1);
    if ((WaveGetLaneIndex() == 10)) {
      result = (result + WaveActiveMin(1));
    }
    if ((WaveGetLaneIndex() == 0)) {
      result = (result + WaveActiveMax(result));
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
    }
  case 1: {
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        uint counter2 = 0;
        while ((counter2 < 3)) {
          counter2 = (counter2 + 1);
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
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
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
  for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
    if ((WaveGetLaneIndex() == 11)) {
      result = (result + WaveActiveSum(result));
    }
    if ((WaveGetLaneIndex() == 15)) {
      if ((WaveGetLaneIndex() == 12)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
      }
    }
    if ((WaveGetLaneIndex() == 15)) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
    }
  }
}
