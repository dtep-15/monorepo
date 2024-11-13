use std::future::Future;
use std::sync::Arc;
use std::time::Duration;

use gloo_timers::future::sleep;

pub async fn retry<F, T>(mut f: impl FnMut() -> F) -> T
	where F: Future<Output = anyhow::Result<T>>
{
	loop {
		match f().await {
			Ok(res) => break res,
			Err(err) => {
				log::error!("API request failed: `{}`", err);
				sleep(Duration::from_secs(1)).await;
			},
		}
	}
}

pub mod state {
	#[derive(Debug, Clone, serde::Deserialize)]
	pub struct DeviceState {
		pub is_configured: bool,
	}

	pub async fn state() -> anyhow::Result<DeviceState> {
		Ok(DeviceState { is_configured: true })
	}
}

pub mod networks {
    use super::*;

	#[derive(Debug, Clone, serde::Deserialize)]
	pub struct Network {
		pub name: Arc<str>,
		pub password_required: bool,
	}
	
	pub async fn networks() -> anyhow::Result<Vec<Network>> {
		sleep(Duration::from_secs(1)).await;
		Ok(vec![
			Network {
				name: "pseudo eduroam".into(),
				password_required: true,
			},
			Network {
				name: "Aalto open".into(),
				password_required: false,
			},
			Network {
				name: "Pixel hotspot".into(),
				password_required: true,
			},
			Network {
				name: "lorem ipsum dolor sit amet lorem ipsum dolor".into(),
				password_required: true,
			},
		])
	}

	pub async fn connect(connection_request: NetworkConnection) -> anyhow::Result<Result<(), ()>> {
		_ = connection_request;
		Ok(Ok(()))
	}

	#[derive(Debug, Clone, serde::Serialize)]
	pub struct NetworkConnection {
		pub name: Arc<str>,
		pub password: Option<Arc<str>>,
	}
}

pub mod schedule {
	use super::*;

	#[derive(Debug, Clone, Copy, serde::Deserialize, serde::Serialize)]
	pub struct Schedule {
		// time in minutes
		open_at: usize,
		close_at: usize,
	}
	
	impl Schedule {
		pub fn new(open_at: Time, close_at: Time) -> Self {
			Schedule {
				open_at: open_at.total_minutes(),
				close_at: close_at.total_minutes()
			}
		}

		pub fn open_at(self) -> Time {
			Time {
				minutes: self.open_at
			}
		}

		pub fn close_at(self) -> Time {
			Time {
				minutes: self.close_at
			}
		}
	}

	#[derive(Debug, Clone, Copy)]
	pub struct Time {
		minutes: usize,
	}

	impl Time {
		pub fn new(hours: usize, minutes: usize) -> Self {
			assert!(hours < 24);
			assert!(minutes < 60);

			Time {
				minutes: hours * 60 + minutes
			}
		}

		pub fn minutes(self) -> usize {
			self.minutes % 60
		}

		pub fn hours(self) -> usize {
			self.minutes / 60
		}

		pub fn total_minutes(self) -> usize {
			self.minutes
		}
	}

	impl From<(usize, usize)> for Time {
		fn from(value: (usize, usize)) -> Self {
			let (hours, minutes) = value;

			Time::new(hours, minutes)
		}
	}

	pub async fn schedule() -> anyhow::Result<Schedule> {
		sleep(Duration::from_secs(1)).await;
		Ok(Schedule::new((6, 00).into(), (19, 00).into()))
	}

	pub async fn set_schedule(schedule: Schedule) -> anyhow::Result<()> {
		_ = schedule;
		Ok(())
	}
}
