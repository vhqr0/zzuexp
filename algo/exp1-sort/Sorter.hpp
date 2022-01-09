#ifndef SORTER_HPP
#define SORTER_HPP

#include <functional>

template <typename T> class Sorter {
private:
  std::function<bool(T, T)> comparetor;

public:
  Sorter(std::function<bool(T, T)> comparetor = [](T a, T b) -> bool {
    return a <= b;
  })
      : comparetor(comparetor) {}

  void heapify(T *a, int size);
  void heapify(T *a, int size, int i);
  void heap_sort(T *a, int size);
  void quick_sort(T *a, int size);
};

template <typename T> void Sorter<T>::heapify(T *a, int size) {
  for (int i = (size >> 1) - 1; i >= 0; i--)
    heapify(a, size, i);
}

template <typename T> void Sorter<T>::heapify(T *a, int size, int i) {
  int lci = (i << 1) + 1, rci = lci + 1;
  if (lci >= size)
    return;
  if (rci >= size) {
    if (comparetor(a[i], a[lci])) {
      std::swap(a[i], a[lci]);
      heapify(a, size, lci);
    }
    return;
  }
  if (comparetor(a[i], a[lci]) || comparetor(a[i], a[rci])) {
    if (comparetor(a[lci], a[rci])) {
      std::swap(a[i], a[rci]);
      heapify(a, size, rci);
    } else {
      std::swap(a[i], a[lci]);
      heapify(a, size, lci);
    }
  }
}

template <typename T> void Sorter<T>::heap_sort(T *a, int size) {
  heapify(a, size);
  for (int i = size - 1; i >= 1; i--) {
    std::swap(a[0], a[i]);
    heapify(a, i, 0);
  }
}

template <typename T> void Sorter<T>::quick_sort(T *a, int size) {
  if (size <= 1)
    return;
  int i = 0, j = size - 1;
  while (i < j) {
    while (i < j && comparetor(a[i], a[j]))
      j--;
    if (i != j) {
      std::swap(a[i], a[j]);
      i++;
    }
    while (i < j && comparetor(a[i], a[j]))
      i++;
    if (i != j) {
      std::swap(a[i], a[j]);
      j--;
    }
  }
  quick_sort(a, i);
  quick_sort(a + i + 1, size - i - 1);
}

#endif
