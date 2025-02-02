import hashlib
import math


class BloomFilter:
  def __init__(self, size, hash_count):
      self.size = size
      self.hash_count = hash_count
      self.bit_array = [0] * size

  def add(self, item):
      for i in range(self.hash_count):
          digest = int(hashlib.md5((item + str(i)).encode('utf-8')).hexdigest(), 16)
          index = digest % self.size
          self.bit_array[index] = 1

  def check(self, item):
      for i in range(self.hash_count):
          digest = int(hashlib.md5((item + str(i)).encode('utf-8')).hexdigest(), 16)
          index = digest % self.size
          if self.bit_array[index] == 0:
              return False
      return True


def load_bloom_filter(file_path, bloom_filter):
  with open(file_path, 'r', encoding='ISO-8859-1') as file:
      for line in file:
          bloom_filter.add(line.strip())

rockyou_file = 'rockyou.ISO-8859-1.txt'
bloom_filter = BloomFilter(size=1000000, hash_count=10)  # Example parameters
load_bloom_filter(rockyou_file, bloom_filter)


def test_dictionary(file_path, bloom_filter):
  results = {'true_positive': 0, 'true_negative': 0, 'false_positive': 0, 'false_negative': 0}
  with open(file_path, 'r', encoding='ISO-8859-1') as file:
      for line in file:
          word = line.strip()
          if bloom_filter.check(word):
              if word in rockyou_words:  # Assume rockyou_words is a set of all words in rockyou.txt
                  results['true_positive'] += 1
              else:
                  results['false_positive'] += 1
          else:
              if word in rockyou_words:
                  results['false_negative'] += 1
              else:
                  results['true_negative'] += 1
  return results

dictionary_file = 'dictionary.txt'
rockyou_words = set(open(rockyou_file, 'r', encoding='ISO-8859-1').read().splitlines())
results = test_dictionary(dictionary_file, bloom_filter)
print(results)


print("True Positives:", results['true_positive'])
print("True Negatives:", results['true_negative'])
print("False Positives:", results['false_positive'])
print("False Negatives:", results['false_negative'])
