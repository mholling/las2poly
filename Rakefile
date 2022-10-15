require "json"
require "tmpdir"

desc "generate README from manpage"
file "README.md" => "man1/las2land.1" do |md|
  sh %Q[mandoc -mdoc -T lint man1/las2land.1]
  sh %Q[mandoc -mdoc -T markdown man1/las2land.1 > README.md]
end

task default: "README.md"

desc "create the build directory"
file "build" do
  sh %Q[cmake -S . -B build -DCMAKE_BUILD_TYPE=Release]
end

desc "build las2land binary"
file "bin/las2land" => FileList["build", "src/*", "CMakeLists.txt"] do
  sh %Q[cmake --build build]
  sh %Q[cmake --install build --prefix .]
end

desc "clean build and output directories"
task :clean do
  rm_rf "build"
  rm_rf "share"
  rm_rf "bin"
end

desc "rebuild when changes detected"
task :watch do
  sh %Q[ls src/* man1/* CMakeLists.txt | entr -d -s 'rake bin/las2land README.md']
end

desc "test build with gcc"
task :gcc do
  sh %Q[g++-mp-10 -O3 -std=c++2a -Wfatal-errors -Wall -Wextra -Wpedantic -o /dev/stdout src/main.cpp | wc -c]
end

desc "download WKT strings and generate src/wkts.hpp"
file "src/wkts.hpp" do |hpp|
  epsgs, pairs, wkts = Queue.new, Queue.new, Array[]
  12.times.map do
    Thread.new do
      while epsg = epsgs.pop
        $stderr.print "\r#{pairs.size} done, #{epsgs.size} remain "
        wkt = %x[curl --silent -X GET 'https://apps.epsg.org/api/v1/CoordRefSystem/#{epsg}/export/?format=wkt&formatVersion=1']
        pairs << [epsg, wkt]
      end
    end
  end.tap do
    (0..).each do |page|
      json = %x[curl --silent -X GET --header 'Accept: application/json' 'https://apps.epsg.org/api/v1/ProjectedCoordRefSystem/?page=#{page}&pageSize=1000']
      break epsgs unless JSON.parse(json).dig("Results").each do |result|
        epsgs << result["Code"]
      end.any?
    end.close
  end.each(&:join)
  $stderr.puts
  while !pairs.empty?
    wkts << pairs.pop
  end
  wkts.sort_by(&:first).map do |epsg, wkt|
    %Q[\t{#{epsg}, #{wkt.inspect}},]
  end.join(?\n).tap do |entries|
    File.write hpp.name, <<~CPP
      ////////////////////////////////////////////////////////////////////////////////
      // Copyright 2021 Matthew Hollingworth.
      // Distributed under GNU General Public License version 3.
      // See LICENSE file for full license information.
      ////////////////////////////////////////////////////////////////////////////////

      #ifndef WKTS_HPP
      #define WKTS_HPP

      #include <utility>

      std::pair<int, char const *> static const wkts[] = {
      #{entries}
      };

      #endif
    CPP
  end
end
