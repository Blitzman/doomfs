#ifndef PPM_WRITER_HPP
#define PPM_WRITER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

class PPMWriter
{
  public:

		template <typename T>
    void write(const std::vector<T> & colors, int rows, int cols, std::string filename)
    {
      std::ofstream f_(filename);

      if (f_.is_open())
      {
        f_ << "P3\n" << cols << " " << rows << "\n255\n";
        
        for (int j = 0; j < rows; ++j)
        {
          for (int i = 0; i < cols; ++i)
          {
            int idx_ = j * cols + i;

            int ir_ = colors[idx_].r;
            int ig_ = colors[idx_].g;
            int ib_ = colors[idx_].b;

            f_ << ir_ << " " << ig_ << " " << ib_ << "\n";
          }
        }

        f_.close();

				std::cout << "Written file " << filename << "\n";
      }
      else
      {
        std::cerr << "ERROR: Unable to open file " << filename << "\n";
      }
    }
};

#endif
