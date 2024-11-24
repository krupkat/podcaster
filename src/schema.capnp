@0x921855bb506414ab;

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("podcaster");

struct PlaybackProgress {
  union {
    done @0 :Void;
    inProgress :group {
      elapsedSeconds @1 :Int32;
      totalSeconds @2 :Int32;
    }
  }
}

struct DownloadProgress {
  union {
    done @0 :Void;
    inProgress :group {
      downloadedBytes @1 :Int32;
      totalBytes @2 :Int32;
    }
  }
}

interface Session(ProgressType) {
  pause @0 ();
  resume @1 ();
  close @2 ();
  progress @3 () -> (progress :ProgressType);
}

interface Episode {
  struct Entry {
    title @0 :Text;
    description @1 :Text;
  }

  download @0 () -> (progress :Session(DownloadProgress));
  play @1 () -> (progress :Session(PlaybackProgress));
}

struct Podcast {
  title @0 :Text;
  episodes @1 :List(Episode.Entry);
}

interface PodcasterService {
  refresh @0 () -> (podcasts :List(Podcast));
  connect @1 (podcast :Text) -> (episode :Episode);
}
