use std::future::Future;
use std::sync::{Arc, OnceLock};
use std::time::Duration;

use cfg_if::cfg_if;
use gloo_timers::future::sleep;

static HTTP: OnceLock<reqwest::Client> = OnceLock::new();
static URI_BASE: OnceLock<Arc<str>> = OnceLock::new();

pub fn init(base_uri: Arc<str>) {
	URI_BASE.get_or_init(|| base_uri);
}

// reqwest doesn't support relative URIs
fn uri(api_uri: &str) -> String {
	format!("{}{}", URI_BASE.get().expect("API client uninitialized"), api_uri)
}

fn http() -> &'static reqwest::Client {
	HTTP.get_or_init(|| {
		reqwest::Client::new()
	})
}

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
	use super::*;

	#[derive(Debug, Clone, serde::Deserialize)]
	pub struct DeviceState {
		pub is_configured: bool,
	}

	pub async fn state() -> anyhow::Result<DeviceState> {
		cfg_if! {
			if #[cfg(feature = "mock-api")] {
				Ok(DeviceState { is_configured: true })
			} else {
				Ok(
					http().get(uri("/api/state"))
						.send().await?
						.error_for_status()?
						.json().await?
				)
			}
		}
	}
}

pub mod toggle {
	use super::*;

	pub async fn toggle() -> anyhow::Result<()> {
		cfg_if! {
			if #[cfg(feature = "mock-api")] {
				Ok(())
			} else {
				http().post(uri("/api/toggle"))
					.send().await?
					.error_for_status()?
			}
		}
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
		cfg_if! {
			if #[cfg(feature = "mock-api")] {
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
			} else {
				Ok(
					http().get(uri("/api/networks/"))
						.send().await?
						.error_for_status()?
						.json().await?
				)
			}
		}

	}

	pub async fn connect(connection_request: NetworkConnection) -> anyhow::Result<Result<(), ()>> {
		cfg_if! {
			if #[cfg(feature = "mock-api")] {
				Ok(Ok(()))
			} else {
				let res = http().post(uri("/api/networks/connect"))
					.json(&connection_request)
					.send().await?;

				// outer result - network failures and invalid JSON
				// inner result - the device failed to connect to the network (status code = 403)
				if res.status() == 403 {
					Ok(Err(()))
				} else {
					res.error_for_status()?;
					Ok(Ok(()))
				}
			}
		}
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
		cfg_if! {
			if #[cfg(feature = "mock-api")] {
				sleep(Duration::from_secs(1)).await;
				Ok(Schedule::new((6, 00).into(), (19, 00).into()))
			} else {
				Ok(http().get(uri("/api/schedule"))
					.send().await?
					.error_for_status()?
					.json().await?)
			}
		}
	}

	pub async fn set_schedule(schedule: Schedule) -> anyhow::Result<()> {
		cfg_if! {
			if #[cfg(feature = "mock-api")] {
				_ = schedule;
			} else {
				http().post(uri("/api/schedule"))
					.json(&schedule)
					.send().await?
					.error_for_status()?;
			}
		}

		Ok(())
	}
}
