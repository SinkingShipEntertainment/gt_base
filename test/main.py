


def main():
  '''
  '''

  dirPath = 'C:/Users/tom/Downloads/test'

  for i in range(1, 13):
    # print(f'{i:04d}')

    filePath = f'{dirPath}/JUB108_130030.AP01.rec709.v002.{i:04d}.txt'

    with open(filePath, 'w') as f:
      f.write('test')


if __name__ == '__main__':
  main()
