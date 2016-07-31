#!/usr/bin/env ruby

require 'pp'

class Centroid
  AXIS = [-2.0, -1.0, 0.0, 1.0, 2.0]

  attr_reader :input
  def initialize(input)
    @input = input
  end

  def run
    line = input.gets
    points = line.split(" ").map(&:to_i).map(&:to_f)

    segment_centers = []

    (AXIS.count - 1).times do |index|
      segment_centers << calculate_x_centroid(AXIS[index], points[index], AXIS[index + 1], points[index +1])
    end

    total_weight = 0.0
    center_x = 0.0
    segment_centers.each do |(x_pos, weight)|
      total_weight += weight
      center_x += x_pos*weight
    end
    center_x = (center_x/total_weight)

    puts "The centroid center is: #{'%0.02f' % [center_x]}"
  end

  def calculate_x_centroid(x1, y1, x2, y2)
    base = x2 - x1

    rect_height = [y1, y2].min
    rect_weight = (rect_height * base).abs
    rect_mid = (x1+x2)/2.0

    triangle_height = [y1,y2].max - rect_height
    triangle_weight = (1.0/2.0*triangle_height*base).abs
    if y1 > y2
      triangle_mid = x1 + (x2-x1)/3.0
    elsif y2 > y1
      triangle_mid = x2 - (x2-x1)/3.0
    elsif y2 == y1
      triangle_mid = 0
    else
      raise "All cases should have been covered"
    end

    total_weight = triangle_weight + rect_weight

    if total_weight == 0
      [rect_mid, 0]
    else
      [(rect_mid*rect_weight + triangle_mid*triangle_weight)/total_weight, total_weight]
    end
  end
end


Centroid.new(STDIN).run
