[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if ((WaveGetLaneIndex() == 7)) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
  } else {
  if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14))) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
  } else {
  if ((WaveGetLaneIndex() == 9)) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
  } else {
  if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveMin(5));
  }
  }
  }
  }
  }
  if ((WaveGetLaneIndex() == 6)) {
    if ((WaveGetLaneIndex() == 0)) {
      result = (result + WaveActiveSum(result));
    }
    for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveMin(6));
        }
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
        }
      }
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
        result = (result + WaveActiveMax(result));
      }
      if ((i0 == 1)) {
        continue;
      }
    }
    if ((WaveGetLaneIndex() == 8)) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
    }
  } else {
  if ((WaveGetLaneIndex() == 8)) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
  }
  for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
    }
    uint counter3 = 0;
    while ((counter3 < 3)) {
      counter3 = (counter3 + 1);
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveMin(result));
      }
      if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
      }
    }
    if ((i2 == 1)) {
      continue;
    }
  }
  if ((WaveGetLaneIndex() == 12)) {
    result = (result + WaveActiveMin(WaveGetLaneIndex()));
  }
  }
}
